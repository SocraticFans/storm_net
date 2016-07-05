#ifndef SOCKET_DEFINE_H
#define SOCKET_DEFINE_H

#include <stdint.h>
#include <list>
#include "io_buffer.h"

namespace storm {

class Socket;
enum SocketCloseType {
	CloseType_Self = 0,			// 本方主动断开
	CloseType_Peer = 1,   		// 对端主动断开
	CloseType_Timeout = 2,  	// 超时
	CloseType_IdleTimeout = 3, // 空连接超时
	CloseType_ConnTimeOut = 4,  // 连接超时
	CloseType_PacketError = 5, 	// 协议包出错
	CloseType_ConnectFail = 6, 	// 连接失败
};

inline const char* etos(SocketCloseType closeType) {
	switch (closeType) {
		case CloseType_Self:
			return "Self";
		case CloseType_Peer:
			return "Peer";
		case CloseType_Timeout:
			return "Timeout";
		case CloseType_IdleTimeout:
			return "IdleTimeout";
		case CloseType_ConnTimeOut:
			return "ConnTimeOut";
		case CloseType_PacketError:
			return "PacketError";
		case CloseType_ConnectFail:
			return "ConnectFail";
	}
	return "unknown";
}

enum {
	Packet_Normal,		// 解包成功
	Packet_Less,		// 数据不够
	Packet_Error,		// 解包出错
};

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

}

#endif
