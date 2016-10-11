#pragma once

#include <string>
#include <map>
#include <vector>

#include <time.h>
#include <stdint.h>

#include "event2/event_struct.h" 

#include "IceService.h"
#include "MediaFilter.h"
#include "xrtp_h264.h"

static const int FORWARD_ALIVE_CHECK_TIME = 30;	/**10 minute.*/
static const int MAX_UDP_PACKET_LEN = 10 * 1024;

enum MEDIA_TYPE {
	MEDIA_AUDIO,
	MEDIA_VIDEO
};

static const int AUDIO_RATIO_WEBRTC2XMPP = -3;


/**
¥” ’µΩµƒ ˝æ›∞¸÷–µ√µΩµƒ «µÿ÷∑ «socketµƒ≥ˆø⁄µÿ÷∑°£
ƒø«∞£¨»œŒ™webrtc answer÷–µ√µΩµƒµÿ÷∑∫Õ∆‰≥ˆø⁄ «œ‡Õ¨µƒ£ª 
*/
class ForwardService {
	friend class TestForward;
public:
	struct Address
	{
		std::string	ip;
		uint16_t	port;
		
		bool operator==(const Address &addr) const
		{
			return (addr.ip == ip) && (addr.port == port);
		};
		bool operator<(const Address& addr) const 
		{
            std::string thisIp = this->ip;
            std::string otherIp = addr.ip;
            if(thisIp < otherIp){
                return true;
            }
            if(thisIp > otherIp){
                return false;
            }
            return (port < addr.port);
            
//			return (ip < addr.ip) || (port < addr.port);
		}
	};
	struct Forward {
        uint64_t        sessionId;
		AddrPair		iceInfo;
		MEDIA_TYPE		mediaType;

		sockaddr_in		webrtcAddr;
		sockaddr_in		xmppAddr;
						
		event*			ev_udp;

		char			buffer[MAX_UDP_PACKET_LEN];

		time_t			lastPackageTime;		/**long time(for example: 10min) no package, will terminate it.*/

		VideoRtpFilter		videoFilter;
		VideoResendService	xmppVideoResend;	/** xmppøÕªß∂À”–∂™∞¸÷ÿ∑¢µƒ«Î«Û¥¶¿Ì */
		AudioRtpFilter		audioFilter;
        uint32_t audio_ssrc;
        uint32_t video_ssrc;

	xrtp_h264_repacker video_repacker_;
	xrtp_transformer audio_transformer_;
	unsigned char * nalu_buf_;
	int nalu_buf_size_;

	int64_t start_time_base_;
	int exist_timestamp_;
	int force_build_timestamp;
        
        int64_t toWebrtcPackets;
        int64_t toWebrtcBytes;
        int64_t toV1Packets;
        int64_t toV1Bytes;


		bool isWebrtcAddr(const struct sockaddr_in& tempadd);

		Forward() ;
		~Forward();
	};
	typedef std::map<Address, Forward> ForwardMap;
			
	ForwardService();
	virtual ~ForwardService() ;

	void startForward(uint64_t sessionId, MEDIA_TYPE media, AddrPair& addr_pairs, const sockaddr_in& webrtcAddr);
		
private:
	static void udpCallback(evutil_socket_t fd, short what, void *arg);
	static void timeout_cb(evutil_socket_t fd, short event, void *arg);
		

	Forward* findForward(const Address& local) {
		ForwardMap::iterator it = forwards_.find(local);
		return (it == forwards_.end()) ? NULL : &it->second;
	}

private:	
	struct event	timer_;		
		
	ForwardMap		forwards_;
    uint32_t        current_ssrc_;


};

