#include "event2/event.h"  
#include "event2/event_struct.h"  
#include "event2/util.h"  
#include "event2/buffer.h"  
#include "event2/listener.h"  
#include "event2/bufferevent.h"  

#include <string.h>

#include <assert.h>
#include <arpa/inet.h>

#include "applog.h"
#include "AppServer.h"
#include "ForwardService.h"
#include "xcutil.h"



void ForwardService::startForward(uint64_t sessionId, MEDIA_TYPE media, AddrPair& iceInfo, const sockaddr_in& webrtcAddr)
{
	assert(iceInfo.local_port > 0);
		
	LOG_INFO("Start forward, local " << iceInfo.local_ip << ":" << iceInfo.local_port
		<< "; remote " << iceInfo.remote_ip << ":" << iceInfo.remote_port
		<< "; webrtc addr: " << toHexString((char*)&webrtcAddr, sizeof(sockaddr_in))
        << "; media=" << media
        << "; sid=" << sessionId);

	if (iceInfo.local_port == 0){
		LOG_WARN("error local port " << iceInfo.local_port);
		return;
	}

	int local_fd = (iceInfo.local_fd > 0) ? iceInfo.local_fd : createUdpSocket(iceInfo.local_port);
	if (local_fd <= 0){
		LOG_WARN("error local fd from ice result.");
		return;
	}

	Address local;
	local.ip = iceInfo.local_ip;
	local.port = iceInfo.local_port;	
	if (forwards_.find(local) != forwards_.end()){
		LOG_WARN("This local address has already used!, ip:" << local.ip << ", port:" << local.port);
		return;
	}	
	evutil_make_socket_nonblocking(local_fd);
	
	Forward& forward = forwards_[local];
	
	forward.ev_udp = event_new(g_app.eventBase(), local_fd, EV_READ | EV_PERSIST, udpCallback, &forward);
	if (!forward.ev_udp) {
		LOG_ERROR("Cann't new event for udp transport.");
		forwards_.erase(local);
		return;
	}

	event_add(forward.ev_udp, NULL);

    forward.sessionId = sessionId;
	forward.iceInfo = iceInfo;	
	forward.mediaType = media;
	forward.webrtcAddr = webrtcAddr;	
	memset(&forward.xmppAddr, 0, sizeof(struct sockaddr_in));
	forward.xmppAddr.sin_family = AF_INET;
	forward.xmppAddr.sin_port = htons(forward.iceInfo.remote_port);
	forward.xmppAddr.sin_addr.s_addr = inet_addr(forward.iceInfo.remote_ip.c_str());

	forward.xmppVideoResend.setUdpSendFunctor(std::bind(::sendto, local_fd, std::placeholders::_1, std::placeholders::_2, 0, (struct sockaddr*)&forward.xmppAddr, sizeof(struct sockaddr_in)));	
//	forward.xmppVideoResend.setUdpSendFunctor([=](const char* data, int len){
//		return sendto(local_fd, data, len, 0, (struct sockaddr*)&forward.xmppAddr, sizeof(struct sockaddr_in));
//	});

	
	uint32_t local_ssrc = 0x1234;
	uint32_t local_audio_payloadtype = 120;
	uint32_t local_video_payloadtype = 96;
	int src_samplerate = 16000;
	int dst_samplerate = 48000;

	Forward * obj = &forward;
	xrtp_h264_repacker_init(&obj->video_repacker_
		, obj->nalu_buf_, obj->nalu_buf_size_, 0
		, local_ssrc, local_video_payloadtype, 1412
		, 0, 0);
	xrtp_transformer_init(&obj->audio_transformer_, src_samplerate, dst_samplerate, local_audio_payloadtype, local_ssrc );
	obj->start_time_base_ = 0;
	obj->exist_timestamp_ = 0;
	obj->force_build_timestamp = 1;
    
    obj->toWebrtcPackets = 0;
    obj->toWebrtcBytes = 0;
    obj->toV1Packets = 0;
    obj->toV1Bytes = 0;
}

/// 清理不使用的会话
void ForwardService::timeout_cb(evutil_socket_t fd, short event, void *arg)
{
	assert(arg != NULL);
	time_t curTime = time(NULL);
	ForwardService* fs = (ForwardService*)arg;
	for (ForwardMap::iterator it = fs->forwards_.begin(); it != fs->forwards_.end();)
	{
		Forward& forward = it->second;
		if ((curTime - forward.lastPackageTime) > FORWARD_ALIVE_CHECK_TIME) {
			LOG_INFO("This media session type:" << forward.mediaType				
				<< "; webrtc addr: " << toHexString((char*)&forward.webrtcAddr, sizeof(sockaddr_in))
				<< "; xmpp addr: " << toHexString((char*)&forward.xmppAddr, sizeof(sockaddr_in))
				<< " is not alive for: " << curTime - forward.lastPackageTime << "s, will remove it!");
			fs->forwards_.erase(it++);
		}
		else
			++it;
	}
}


ForwardService::ForwardService() {
	/// create a timer		
	struct timeval tv;
	event_assign(&timer_, g_app.eventBase(), -1, EV_PERSIST, timeout_cb, this);
	evutil_timerclear(&tv);
	tv.tv_sec = FORWARD_ALIVE_CHECK_TIME;
	event_add(&timer_, &tv);
}

ForwardService::~ForwardService() {

}

bool ForwardService::Forward::isWebrtcAddr(const struct sockaddr_in& tempadd)
{
	if (memcmp(&webrtcAddr, &tempadd, sizeof(tempadd)) == 0)
		return true;
	else if (webrtcAddr.sin_port == xmppAddr.sin_port) {
		// do more...
		return false;
	}
	else {
		return tempadd.sin_port == webrtcAddr.sin_port;
	}
}


// #define dbgv(...) do{  printf("<udp_dump>[D] " __VA_ARGS__); printf("\n"); fflush(stdout); }while(0)
// #define dbgi(...) do{  printf("<udp_dump>[I] " __VA_ARGS__); printf("\n"); fflush(stdout); }while(0)
// #define dbge(...) do{  printf("<udp_dump>[E] " __VA_ARGS__); printf("\n"); fflush(stdout); }while(0)
// 
// static 
// void dump_rtp(int packet_count, void* buf_, int len ){
// 	const unsigned char * buf = (const unsigned char *)buf_;
// 	char prefix[16];
// 	sprintf(prefix, "No.%d", packet_count);
// 	if(len < 12){
// 		dbgi("%s rtp len too small %d", prefix, len);
// 		return;
// 	}

// 	unsigned char v =  (buf[0]>>6) & 0x3;
// 	unsigned char p =  (buf[0]>>5) & 0x1;
// 	unsigned char x =  (buf[0]>>4) & 0x1;
// 	unsigned char cc = (buf[0]>>0) & 0xF;
// 	unsigned char m =  (buf[1]>>7) & 0x1;
// 	unsigned char pt = (buf[1]>>0) & 0x7F;
// 	unsigned short seq = be_get_u16(buf+2);
// 	unsigned int ts = be_get_u32(buf+4);
// 	unsigned int ssrc = be_get_u32(buf+8);
	
// 	int header_len = 12 + cc * 4;
// 	if(x){
// 		int min = header_len + 4;
// 		if(min <= len){
// 			int ext_len = be_get_u16(buf+header_len+2);
// 			header_len += 4+ext_len*4;
// 		}else{
// 			header_len = -1;
// 		}
// 	}

// 	dbgi("%s rtp[v=%d,p=%d,x=%d,cc=%d,m=%d,pt=%d,seq=%d,ts=%d,ssrc=%d,pl=%d]", prefix
// 		, v, p, x, cc, m, pt, seq, ts, ssrc, header_len);
// }


static 
inline int buildTimestamp(ForwardService::Forward * forward, int dataLen ){
	unsigned char * buf = (unsigned char *)forward->buffer;

	if(forward->start_time_base_ == 0){
		forward->start_time_base_ = get_timestamp_ms();
	}

	int64_t elapsed = get_timestamp_ms() - forward->start_time_base_;
	unsigned int ts = (unsigned int) (9*elapsed); //90000*elapsed / 1000;
	be_set_u32(ts, buf+4);

	return 0;
}

static 
inline int maybeBuildTimestamp(ForwardService::Forward * forward, int dataLen ){
	unsigned char * buf = (unsigned char *)forward->buffer;

	if(dataLen <= 12){
		return -1;
	}

	if(forward->force_build_timestamp){
		return buildTimestamp(forward, dataLen);
	}

	if(forward->exist_timestamp_){
		return 0;
	}

	// unsigned short seq = be_get_u16(buf+2);
	unsigned int ts = be_get_u32(buf+4);
	if(ts){
		forward->exist_timestamp_ = 1;
		return 0;
	}

	return buildTimestamp(forward, dataLen);
}

void ForwardService::udpCallback(evutil_socket_t fd, short what, void *arg)
{	
	if (!(what&EV_READ)) return;

	assert(arg != NULL);
	Forward * forward = (Forward*)arg;

	struct sockaddr_in tempadd;
	ev_socklen_t addrLen = sizeof(tempadd);
	
	while (true)
	{
		int dataLen = recvfrom(fd, forward->buffer, MAX_UDP_PACKET_LEN, 0, (struct sockaddr*)&tempadd, &addrLen);
		if (dataLen <= 0){
			break;
		}
		forward->lastPackageTime = time(NULL);
		
		//LOG_DEBUG("receive data len: " << dataLen << "; from " << ntohs(tempadd.sin_port));				
		//LOG_DEBUG("from addr: " << toHexString((const char*)&tempadd, addrLen)
		//	<< "; webrtc addr: " << toHexString((char*)&forward->webrtcAddr, addrLen)
		//	<< "; xmpp addr: " << toHexString((char*)&forward->xmppAddr, addrLen));

		// forward received data
		if (forward->isWebrtcAddr(tempadd)) {
			if (forward->mediaType == MEDIA_VIDEO) {				
				if (!forward->videoFilter.filter(forward->buffer, dataLen))
					assert(false);// continue;
				forward->xmppVideoResend.cacheData(forward->buffer, dataLen);

				// 模拟丢包，看看情况； 丢5%
				/*
				struct timeval tpstart;
				gettimeofday(&tpstart, NULL);
				srand(tpstart.tv_usec);
				if (rand() % 20 == 0)
					continue;
					*/
			} 
			else if (forward->mediaType == MEDIA_AUDIO) {
				if (!forward->audioFilter.filter(forward->buffer, dataLen, AUDIO_RATIO_WEBRTC2XMPP))
					assert(false);
				//	continue;
			}
			
			sendto(fd, forward->buffer, dataLen, 0, (struct sockaddr*)&forward->xmppAddr, addrLen);
			LOG_DEBUG("send data len: " << dataLen << "; from addr: " << toHexString((const char*)&tempadd, addrLen) << " to xmpp, port: " << ntohs(forward->xmppAddr.sin_port));
            forward->toV1Packets += 1;
            forward->toV1Bytes += dataLen;
		}
		else {
			unsigned char h = forward->buffer[0];

            if(h == 0xfa){
                // delay (ping) , send back
                sendto(fd, forward->buffer, dataLen, 0, (struct sockaddr*)&tempadd, addrLen);
                continue;
            }
            
            
			if (forward->mediaType == MEDIA_VIDEO) {
				// 如果是xmpp的重发请求，直接处理掉
				if (forward->xmppVideoResend.handle(forward->buffer, dataLen))
					continue;

				if(h > 0x90){
					// printf("drop non-rtp, h=0x%02X, len=%d\n", h, dataLen);
					continue;
				}
				maybeBuildTimestamp(forward, dataLen);
				// dump_rtp(0, forward->buffer, dataLen);
                
				
			} else if (forward->mediaType == MEDIA_AUDIO) {
				if(h > 0x90){
					// printf("drop non-rtp, h=0x%02X, len=%d\n", h, dataLen);
					continue;
				}

				if (!forward->audioFilter.filter(forward->buffer, dataLen, -AUDIO_RATIO_WEBRTC2XMPP))
					continue;
				// dump_rtp(0, forward->buffer, dataLen);
			}
            
			sendto(fd, forward->buffer, dataLen, 0, (struct sockaddr*)&forward->webrtcAddr, addrLen);
			LOG_DEBUG("send data len: " << dataLen << "; from addr: " << toHexString((const char*)&tempadd, addrLen) << " to webtrc, port: " << ntohs(forward->webrtcAddr.sin_port));
            forward->toWebrtcPackets += 1;
            forward->toWebrtcBytes += dataLen;

			// if (forward->mediaType == MEDIA_VIDEO) {
			// 	// 如果是xmpp的重发请求，直接处理掉
			// 	if (forward->xmppVideoResend.handle(forward->buffer, dataLen))
			// 		continue;

			// 	if(h > 0x90){
			// 		// printf("drop non-rtp, h=0x%02X, len=%d\n", h, dataLen);
			// 		continue;
			// 	}

			// 	int ret = xrtp_h264_repacker_input(&forward->video_repacker_, (unsigned char *)forward->buffer, dataLen);
			// 	if(ret > 0){
			// 		int len;
			// 		while ((len = xrtp_h264_repacker_next(&forward->video_repacker_, (unsigned char *)forward->buffer)) > 0) {
			// 			dataLen = len;
			// 			sendto(fd, forward->buffer, dataLen, 0, (struct sockaddr*)&forward->webrtcAddr, addrLen);
			// 			LOG_DEBUG("send data len: " << dataLen << "; from addr: " << toHexString((const char*)&tempadd, addrLen) << " to webtrc, port: " << ntohs(forward->webrtcAddr.sin_port));
			// 		}
			// 	}


			// } else if (forward->mediaType == MEDIA_AUDIO) {
			// 	if(h > 0x90){
			// 		// printf("drop non-rtp, h=0x%02X, len=%d\n", h, dataLen);
			// 		continue;
			// 	}

			// 	xrtp_transformer_process(&forward->audio_transformer_, (unsigned char *)forward->buffer);
			// 	sendto(fd, forward->buffer, dataLen, 0, (struct sockaddr*)&forward->webrtcAddr, addrLen);
			// 	LOG_DEBUG("send data len: " << dataLen << "; from addr: " << toHexString((const char*)&tempadd, addrLen) << " to webtrc, port: " << ntohs(forward->webrtcAddr.sin_port));
			// }

			


		}
	}
}


ForwardService::Forward::Forward() : lastPackageTime(time(NULL)) {
	this->nalu_buf_size_ = 512*1024;
	LOG_INFO("malloc nalu bytes " << this->nalu_buf_size_ );
	this->nalu_buf_ = (unsigned char *) malloc(this->nalu_buf_size_);
}

ForwardService::Forward::~Forward() {
    
    const char * media;
    if(this->mediaType == MEDIA_VIDEO){
        media = "video";
    }else if(this->mediaType == MEDIA_AUDIO){
        media = "audio";
    }else{
        media = "unknown";
    }
    LOG_INFO("toWebrtc: sid=" << this->sessionId << ", m=" << media << ", pkts=" << this->toWebrtcPackets << ", bytes=" << this->toWebrtcBytes);
    LOG_INFO("toV1    : sid=" << this->sessionId << ", m=" << media << ", pkts=" << this->toV1Packets     << ", bytes=" << this->toV1Bytes);
    
	if (iceInfo.local_fd > 0){
		evutil_closesocket(iceInfo.local_fd);
	}
	if (ev_udp != NULL) {
		event_free(ev_udp);
	}
	if(this->nalu_buf_){
		free(this->nalu_buf_);
	}
}

