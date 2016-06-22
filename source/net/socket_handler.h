#ifndef _SOCKET_HANDLER_H_
#define _SOCKET_HANDLER_H_

#include "socket_define.h"

namespace storm {
class SocketLoop;
class SocketHandler {
public:
	SocketHandler(SocketLoop* loop);
	virtual ~SocketHandler() {};

	// 网络线程调用
	virtual void onConnect(Socket* s) {}
	virtual void onAccept(Socket* s) {}
	virtual bool onData(Socket* s);
	virtual void onClose(Socket* s, uint32_t closeType) {}

	// 逻辑调用
	virtual void onPacket(Socket* s, const char* data, uint32_t len) = 0;

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
