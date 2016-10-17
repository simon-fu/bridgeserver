#pragma once

#include <string>
#include <stdint.h>

#include <functional>

#include "applog.h"
#include "util.h"
#include "AppServer.h"
#include "ini.hpp"
#include "xcutil.h"

#pragma pack(1)
struct rtp_hdr
{
	uint16_t cc : 4;			/**< CSRC count                     */
	uint16_t x : 1;				/**< header extension flag          */
	uint16_t p : 1;				/**< padding flag                   */
	uint16_t v : 2;				/**< packet type/version            */
	uint16_t pt : 7;			/**< payload type                   */
	uint16_t m : 1;				/**< marker bit                     */
	uint16_t seq;				/**< sequence number                */
	uint32_t ts;				/**< timestamp                      */
	uint32_t ssrc;				/**< synchronization source         */

	uint32_t timeStamp() const { return ntohl(ts); }
	void setTimeStamp(uint32_t t) { ts = htonl(t); }
	uint16_t sequence() const { return ntohs(seq); }
	//uint32_t ssrc() const { return ntohl(ssrc); }
};
#pragma pack()

/// µ˜Ω⁄¿œœµÕ≥∑¢ÀÕµƒ“Ù∆µ ±¥¡
class AudioRtpFilter
{
public:
	AudioRtpFilter() //: hop_(0), startTime_(0), needModTs_(true)
	{
		// memset(&lastRtpHeader_, 0, sizeof(rtp_hdr));
	}

	bool filter(char* data, int dataLen, int sample_ratio)
	{
		assert(data != NULL);
		if ((unsigned)dataLen < sizeof(rtp_hdr)) {
			LOG_WARN("error rtp package with size: ." << dataLen);
			return true;
		}

		if (sample_ratio == 0) {
			LOG_DEBUG("Need not filter.");
			return true;
		}

		std::string oldTs = toHexString(&data[4], 4);

		//rtp_hdr* curRtpHeader = (rtp_hdr*)&data;
		uint32_t* ps = (uint32_t*)&data[4];
		if (sample_ratio > 0) 
			*ps = htonl(ntohl(*ps)*sample_ratio);
			//curRtpHeader->setTimeStamp(curRtpHeader->timeStamp() * sample_ratio);
		else
			*ps = htonl(ntohl(*ps)/(-sample_ratio));
			//curRtpHeader->setTimeStamp(curRtpHeader->timeStamp() / (-sample_ratio));

//		LOG_TRACE("ratio " << sample_ratio << "; timestamp change from " << oldTs << "; to (" << ntohl(*ps) << ")" << toHexString(&data[4], 4));

		return true;
	}


private:
	// rtp_hdr			lastRtpHeader_;
	// unsigned int	hop_;			/**√ø∏ˆ∞¸µƒ–ƒÃ¯º‰∏Ù*/
	// unsigned int	startTime_;		/**rtp ±¥¡ø™ º ±º‰*/

	// bool			needModTs_;
};


/// µ˜Ω⁄webrtc∑¢π˝¿¥µƒ ”∆µ∞¸
class VideoRtpFilter
{
public:
	/**
	*@ desc 
	*@ return true if this package is valid
	*/
	bool filter(char* data, int dataLen) {
		assert(data != NULL);
		if ((unsigned)dataLen < sizeof(rtp_hdr) + 2)
			return true;

		//rtp_hdr* curRtpHeader = (rtp_hdr*)&data;
		int nal_type = data[sizeof(rtp_hdr)] & 0x1F;
		if (nal_type > 0 && nal_type < 24) {
			//if (nal_type == 5 || nal_type == 1 || nal_type == 7 || nal_type == 9)
			//curRtpHeader->m = 1;			
			data[1] |= 0x80;
			assert((data[1] & 0x80) == 0x80);
		}
		
		return true;
	}
};

#define MAX_CACHE_RTP_PKT_COUNT 256
#define MAX_RTP_BUFFER_SIZE     1600

typedef std::function<int(const uint8_t* data, int dataLen)> UdpSendFunctor;
class VideoResendService {
public:
	VideoResendService() {
        rtpPackets_ = new uint8_t *[MAX_CACHE_RTP_PKT_COUNT];
        for(int i = 0; i < MAX_CACHE_RTP_PKT_COUNT; i++){
            rtpPackets_[i] = new uint8_t[MAX_RTP_BUFFER_SIZE];
            memset(rtpPackets_[i], 0, MAX_RTP_BUFFER_SIZE);
        }
		supportResend_ = g_app.iniFile().top()("GLOBAL")["xmpp_resend_video"] != "false";
		LOG_INFO("Receive video enable: " << supportResend_ );
//		printf("Receive video enable: %d\n", supportResend_);
	}
    virtual ~VideoResendService(){
        if(rtpPackets_){
            for(int i = 0; i < MAX_CACHE_RTP_PKT_COUNT; i++){
                delete[] rtpPackets_[i];
            }
            delete[] rtpPackets_;
        }
    }

	void setUdpSendFunctor(const UdpSendFunctor& send) {
		send_ = send;
	}

	void cacheData(const char* rtpData, int dataLen) {
		assert(rtpData != NULL);
		if (dataLen < 12) {
			LOG_WARN("Invalid rtp package!");
			return;
		}
        
        uint16_t storeSeq = be_get_u16(&rtpData[2]);
		LOG_DEBUG("[resend] cache data with seq: " << storeSeq);
        uint8_t * buf = rtpPackets_[storeSeq % MAX_CACHE_RTP_PKT_COUNT];
        be_set_u32(dataLen, &buf[0]);
        memcpy(&buf[4], rtpData, dataLen);
	}
    
	bool handle(const char* rtpData, int dataLen) {
		if ((rtpData[0] & 0xff) != 0xfb) {			
			return false;
		}
	
		if (!supportResend_)
			return true;		

//		uint16_t startSeq = ntohs(*((uint16_t *)&(rtpData[1])));
//		uint16_t endSeq = ntohs(*((uint16_t *)&(rtpData[3])));
        
        uint16_t startSeq =  be_get_u16(&rtpData[1]);
        uint16_t endSeq = be_get_u16(&rtpData[3]);
        
        
        if (startSeq > endSeq) {
			LOG_WARN("Receive video [resend] invalid require with start seqNo: " << startSeq << ", end seqNo: " << endSeq);
			return true;
		}

		LOG_DEBUG("Receive video [resend] require with start seqNo: " << startSeq << ", end seqNo: " << endSeq);


		for (uint16_t seq = startSeq; seq <= endSeq; seq++)
		{
            uint8_t * buf = rtpPackets_[seq % MAX_CACHE_RTP_PKT_COUNT];
            uint32_t rtpLen = be_get_u32(&buf[0]);
            uint8_t * rtpPkt = buf+4;
            
            uint16_t storeSeq = be_get_u16(&rtpPkt[2]);
            if(rtpLen >= 12 && storeSeq == seq){
                LOG_DEBUG("[resend] video rtp packet seq: " << seq);
                send_(rtpPkt, rtpLen);
            }else{
                LOG_DEBUG("Rtp [resend] cache have not packet with seq: " << seq);
            }
		}

		return true;
	}
private:
	UdpSendFunctor				send_;
    uint8_t **                  rtpPackets_;

	bool						supportResend_;
};


