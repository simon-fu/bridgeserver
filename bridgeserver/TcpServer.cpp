#include "event2/event.h"  
#include "event2/util.h"  
#include "event2/buffer.h"  
#include "event2/listener.h"  
#include "event2/bufferevent.h"  
#include <arpa/inet.h> // inet_addr

#include <iostream>  
#include <string.h>

#include "AppServer.h"
#include "applog.h"
#include "Connection.h"
#include "TcpServer.h"


TcpServer::TcpServer(unsigned short port, event_base *base, ConnectionBuilder* builder)
	: port_(port)
    , base_(base)
	, connectionBuilder_(builder)
{
	assert(builder != NULL);



	
}

TcpServer::~TcpServer()
{
	if (listener_)	evconnlistener_free(listener_);
	delete connectionBuilder_;
}

bool TcpServer::start(){
    
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = htonl(0);
    sin.sin_port = htons(port_);
   
    listener_ = evconnlistener_new_bind(base_, acceptCallback, this, LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, -1, (sockaddr*)&sin, sizeof(struct sockaddr_in));
    if (listener_ == NULL) {
        LOG_ERROR("Fail to start tcp server!");
        return false;
    }
    return true;
}

bool TcpServer::haveConnection(Connection* conn) const
{
	return connections_.find(conn) != connections_.end();
}
void TcpServer::closeConnection(Connection* conn)
{
	std::set<Connection*>::iterator it = connections_.find(conn);
	assert(it != connections_.end());
	//{		LOG_WARN("No this connection!");		return;	}

	const struct sockaddr_in * addrin = (const struct sockaddr_in *) conn->getRemoteAddr();
	const char *ip = inet_ntoa(addrin->sin_addr);
	int port = ntohs(addrin->sin_port);
	LOG_INFO("rpc disconnected from " << ip << ":" << port);


    Connection * conn1 = *it;
	connections_.erase(it);
	delete conn1;
}

void TcpServer::acceptCallback(struct evconnlistener *listener, evutil_socket_t fd, struct sockaddr *addr, int socklen, void *arg)
{
	assert(arg != NULL);

	struct sockaddr_in * addrin = (struct sockaddr_in *) addr;
	const char *ip = inet_ntoa(addrin->sin_addr);
	int port = ntohs(addrin->sin_port);
	LOG_INFO("rpc connected from " << ip << ":" << port);

	TcpServer *self = (TcpServer*)arg;

	bufferevent *bev = bufferevent_socket_new(g_app.eventBase(), fd, BEV_OPT_CLOSE_ON_FREE);
	Connection *conn = self->connectionBuilder_->create(bev, addrin);
	if (conn == NULL) {
		LOG_WARN("No enough memory to new a connection.");
		return;
	}
    conn->setServer(self);
	self->connections_.insert(conn);
}
