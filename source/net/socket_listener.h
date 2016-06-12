#ifndef SOCKET_LISTENER_H_
#define SOCKET_LISTENER_H_

#include <functional>
#include "socket_loop.h"

namespace storm {

class SocketListener : public SocketHandler {
public:
	SocketListener(SocketLoop* loop);
	virtual ~SocketListener(){}

	//virtual void onAccept(Socket* s);
	//virtual void onClose(Socket* s, uint32_t closeType);

	virtual bool onData(Socket* s);
	virtual void onPacket(Socket* s, const char* data, uint32_t len) {}

	typedef int32_t (*PacketParser)(IoBuffer*, const char*& data, uint32_t& len);

	void setPacketParser(PacketParser parser) {
		m_parser = parser;
	}

protected:
	SocketLoop*  m_loop;			// SocketLoop对象
	PacketParser m_parser;			// 分包解析器
};

}

#endif
