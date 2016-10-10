#pragma once

#include <string>
#include <map>

#include <stdint.h>

#include "event2/event_struct.h" 

#include "json/json.h"
#include "eice.h"
#include "IceConnection.h"

struct IceCommand;


static const unsigned MAX_COMMAND_LEN = 10 * 1024;
static const unsigned SESSION_ALIVE_CHECK_TIME = 60;	/**60s.*/

/**construct a command to the buffer with the input parameters.*/
bool constructCommandBuffer(char* buffer, ICE_OP op, SessionID id, const char* data, uint32_t dataLen);

class AddrPair{
public:
	AddrPair(){}
	AddrPair(const std::string & _local_ip, int _local_port, const std::string & _remote_ip, int _remote_port, int _local_fd)
		: local_ip(_local_ip)
		, local_port(_local_port)
		, remote_ip(_remote_ip)
		, remote_port(_remote_port)
		, local_fd(_local_fd)
	{}

	std::string local_ip;
	int			local_port;
	std::string remote_ip;
	int			remote_port;
	int			local_fd;
};

struct IceSession{
	SessionID	id;				/** id of this session.*/
	eice_t		endpoint;		/** as caller or callee of ice*/
	Connection*	request;		/** where the session come from.*/
	int			curStatus;		/** current status of this session*/
	time_t		curTime;		/** the last time of status modified.*/

	char		iceResult[MAX_COMMAND_LEN];	/** result of ice negotiate*/
	int			iceResultLen;

	IceSession() : id(0), endpoint(NULL), request(NULL), curStatus(ICE_OP_NON), iceResultLen(0) {}
	~IceSession() {
		if (endpoint != NULL) {
			eice_free(endpoint);
		}
	}

	void setStatus(ICE_OP status) {
		curStatus = status;
		curTime = time(NULL);
	}
};

typedef std::map<SessionID, IceSession> IceSessionMap;

class IceService {
public:
	IceService();

	void handleIceCommand(const IceCommand &command, Connection* sender);

	void sendCommand(IceSession& is, ICE_OP op, const char* data, uint32_t dataLen);

	void getNegoResult(eice_t obj);

	const char* iceJsonConfig();
    std::string& remoteContent();

private:
	SessionID genSessionId() { static SessionID id = 0; return ++id; }
		
	IceSession* findSesssionByEp(eice_t ep);

	/**
	*@desc ∑÷Œˆ ’µΩµƒ ˝æ›£¨”–“ª–©Ãÿ∂®µƒ ˝æ›–Ë“™¥¶¿Ì
	*param[in] json:  ’µΩµƒjson∏Ò Ω ˝æ›
	* Àµ√˜£∫ ƒø«∞‘⁄ø™ º ±(prepare/join), –Ë“™”–“ª∏ˆ–Ë“™–≠…Ãµƒ“µŒÒ ˝¡ø£®compCount£©
	*/
	void readJsonInfo(const std::string& json, bool isCaller = false);

	static void timeout_cb(evutil_socket_t fd, short event, void *arg);

private:	
	IceSessionMap	sessions_;

	struct event	timer_;							/** timer for checking session.*/

	std::string		iceJsonConfig_;					/**include ice server info*/
	Json_em::Value	configInfo_;

	std::string		testIP_;						/** Œ™≤‚ ‘∑Ω±„£¨‘À––‘⁄≈‰÷√Œƒº˛÷–÷∏∂®≤‚ ‘µÿ÷∑£¨◊¢“‚£∫‘⁄ µº π§◊˜÷–“ª∂®“™»•µÙ’‚∏ˆ≈‰÷√£° */

	char			sendBuffer_[MAX_COMMAND_LEN];	/** command buffer for send data to service requester*/
    
    bool            isCaller_;
    bool            isExistRelayMS_;
    std::string		remoteContent_;
    Json_em::Value  remoteContentValue_;
    
};

