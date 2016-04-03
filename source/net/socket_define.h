#ifndef SOCKET_DEFINE_H
#define SOCKET_DEFINE_H

#include <stdint.h>
#include <list>
#include "io_buffer.h"

namespace storm {

enum SocketType {
	SocketType_Normal = 0,	// 普通连接
	SocketType_Listen, 		// 监听连接
	SocketType_Notify,		// 激活epoll用的
};

enum SocketStatus {
	SocketStatus_Idle,			// 空闲(未被使用)
	SocketStatus_Reserse,		// 保留待使用
	SocketStatus_Listen,		// 监听中
	SocketStatus_Connecting,	// 连接中
	SocketStatus_Connected, 	// 已连接
};

enum SocketCloseType {
	CloseType_Self = 0,			// 本方主动断开
	CloseType_Peer = 1,   		// 对端主动断开
	CloseType_Timeout = 2,  	// 超时
	CloseType_EmptyTimeout = 3, // 空连接超时
	CloseType_ConnTimeOut = 4,  // 连接超时
	CloseType_PacketError = 5, 	// 协议包出错
	CloseType_ConnectFail = 6, 	// 连接失败
};

// listen和connect搞单独线程安全的接口
//

enum SocketCmdType {
	SocketCmd_Connect,
	SocketCmd_Listen,
	SocketCmd_Send,
	SocketCmd_Close,
	SocketCmd_Exit,
};

struct SocketCmd {
	SocketCmd()
		:type(SocketCmd_Send),
	 	 id(0),
		 closeType(CloseType_Self),
		 buffer(NULL) {}

	int32_t type;
	int32_t	id;
	int32_t closeType;
	IoBuffer* buffer;
};

class Socket;
class SocketHandler {
public:
	virtual ~SocketHandler() {};

	virtual void onConnect(Socket* s) = 0;
	virtual bool onAccept(Socket* s) = 0;
	virtual bool onData(Socket* s) = 0;
	virtual void onClose(Socket* s, uint32_t closeType) = 0;
};

struct Socket {
	Socket()
		:id(0),
	 	 fd(-1), 
		 type(SocketType_Normal),
		 status(SocketStatus_Idle), 
		 handler(NULL),
		 readBuffer(NULL) {}

	int32_t id;
	int32_t fd;
	int32_t type;
	int32_t status;
	SocketHandler* handler;
	string ip;
	int32_t port;

	IoBuffer* readBuffer;
	std::list<IoBuffer*> writeBuffer;
};

}

#endif
