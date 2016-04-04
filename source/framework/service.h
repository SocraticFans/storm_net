#ifndef STORM_SERVICE_H_
#define STOMR_SERVICE_H_

#include "net/socket_loop.h"
#include "listener.h"

namespace storm {

struct Connection {
	Connection(int32_t _id, int32_t _fd, string _ip, int32_t _port)
		:id(_id), fd(_fd), port(_port), ip(_ip) {}
	int32_t id;
	int32_t fd;
	int32_t port;
	string ip;
};

// 代码生成类继承他
class Service {
public:
	Service(SocketLoop* loop, Listener* listener);
	virtual ~Service(){}

	//自定义协议重载doRequest
	virtual void doRequest(const Connection& conn, const char* data, uint32_t len);
	virtual void doClose(const Connection& conn) {}

	//RPC协议重载doRpcRequest
//	virtual int doRpcRequest(Connection::ptr conn, const RpcRequest& req, RpcResponse& resp) {
//		return 0;
//	}

protected:
	SocketLoop* m_loop;		// loop对象
	Listener* m_listener;	// 监听器
};

}

#endif
