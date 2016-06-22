#ifndef _STORM_PROXY_MANAGER_H_
#define _STORM_PROXY_MANAGER_H_

#include "net/socket_loop.h"
#include "net/socket_handler.h"

namespace storm {

class StormProxyObject : public SocketHandler {
public:
	StormProxyObject(SocketLoop* loop)
	 :SocketHandler(loop) {

	 }
	virtual ~StormProxyObject() {}

	virtual void onConnect(Socket* s) {m_isActive = true;}
	virtual void onClose(Socket* s, uint32_t closeType) {}

	virtual void onPacket(Socket* s, const char* data, uint32_t len) = 0;

private:
	bool m_isContainer;		// 是否只是一个Proxy容器
	std::string m_ip;		// ip
	uint32_t m_port;		// port
	int32_t m_socketId;		// socketId
	bool m_isActive;		// 是否活跃

};

class StormProxyManager {
public:

};

}

#endif
