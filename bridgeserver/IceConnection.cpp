
#include <assert.h>

#include "applog.h"
#include "AppServer.h"
#include "IceService.h"
#include "util.h"
#include "IceConnection.h"

#define enumValue(key) case key: 	return #key;
std::string enumKey(ICE_OP value)
{
	switch (value)
	{
		enumValue(ICE_OP_NON);
		enumValue(ICE_OP_PREPARE_SESSION);
		enumValue(ICE_OP_LOCAL_INFO);
		enumValue(ICE_OP_START_SESSION);
		enumValue(ICE_OP_JOIN_SESSION);
		enumValue(ICE_OP_ICE_RESULT);
		enumValue(ICE_OP_RELAY_SESSION);
		enumValue(ICE_OP_STOP_SESSION);
		enumValue(ICE_OP_END);
	default:
		return "";
	}
}

Connection* IceConnectionBuilder::create(bufferevent *bev, xsockaddr * addr)
{
	return new(std::nothrow) IceConnection(bev, addr);
}

void IceConnection::handleIceCommand(const IceCommand &command)
{
	g_app.iceService()->handleIceCommand(command, this);
}

void IceConnection::handleCommand(const char *pDataBuffer, int dataLen)
{
	assert(pDataBuffer != NULL && dataLen > 0);

	LOG_INFO("Received data " << dataLen << "; current buffer " << recvBuffer_.size());
	LOG_DEBUG("Received data: " << toHexString(pDataBuffer, std::min(16, dataLen + ICE_COMMAND_HEADER_LEN), " "));

	unsigned totalUsedLen = 0;
	unsigned usedLen = 0;
	recvBuffer_.append(pDataBuffer, dataLen);
	IceCommand response;
	while ((dataLen > (int)totalUsedLen))
	{
		if (!parseCommand(response, recvBuffer_.data() + totalUsedLen, recvBuffer_.size() - totalUsedLen, usedLen))
		{
			break;
		}		
		totalUsedLen += usedLen;

        LOG_INFO("=> handleIceCommand " << response.op);
		handleIceCommand(response);
        LOG_INFO("<= handleIceCommand " << response.op
                 << ", dataLen=" << dataLen
                 << ", totalUsedLen=" << totalUsedLen
                 << ", usedLen=" << usedLen
                 << ", recvBuffer_.size()=" << recvBuffer_.size()
                 );
	}
	assert(totalUsedLen <= recvBuffer_.size());

	// remove used data
	LOG_INFO("buffer len: " << recvBuffer_.size() << "; used " << totalUsedLen);
	if (totalUsedLen == recvBuffer_.size())
	{
		recvBuffer_.clear();
	}
	else
	{
		recvBuffer_.erase(0, totalUsedLen);
	}
	LOG_INFO("current buffer len " << recvBuffer_.size());
}

bool IceConnection::parseCommand(IceCommand &response, const char *pDataBuffer, unsigned nLength, unsigned & usedLen)
{
	assert(pDataBuffer != NULL && nLength > 0);
	
	if (nLength < ICE_COMMAND_HEADER_LEN) {
		LOG_DEBUG("receive data with length is little to " << ICE_COMMAND_HEADER_LEN);
		return false;
	}
	
	uint32_t contentLen = *(unsigned short*)&pDataBuffer[ICE_COMMAND_HEADER_LEN-4];
	if (contentLen > nLength - ICE_COMMAND_HEADER_LEN) {
		LOG_DEBUG("current received data(data len: " << nLength - ICE_COMMAND_HEADER_LEN << ", need: " << contentLen << ") is not enough to make a command.");
		return false;
	}

	usedLen = ICE_COMMAND_HEADER_LEN + contentLen;
	response.op = *(uint32_t*)pDataBuffer;
	response.sessionId = *(SessionID*)(pDataBuffer+4);
	response.content.assign(pDataBuffer+ICE_COMMAND_HEADER_LEN, contentLen);

	return true;	
}

