#ifndef STORM_LISTENER_H_
#define STORM_LISTENER_H_

#include "net/socket_loop.h"
#include "net/socket_listener.h"

namespace storm {
class Service;
class Listener : public SocketListener {
public:
	Listener(SocketLoop* loop, bool inLoop = true);
	virtual ~Listener(){}

	virtual void onAccept(Socket* s);
	virtual void onClose(Socket* s, uint32_t closeType);
	virtual void onPacket(Socket* s, const char* data, uint32_t len);

	void setService(Service* service) {
		m_service = service;
	}

protected:
	bool m_inLoop;					// 逻辑是否在loop线程处理
	Service* m_service;				// 服务(具体逻辑处理器)
};

}

#endif
