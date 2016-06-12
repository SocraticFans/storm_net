#ifndef STORM_SERVICE_H_
#define STOMR_SERVICE_H_

#include <thread>

#include "net/socket_loop.h"
#include "util/util_thread.h"
#include "connection.h"

namespace storm {
class StormListener;

// 代码生成类继承他
class StormService {
public:
	StormService(SocketLoop* loop, StormListener* listener);
	virtual ~StormService(){}

	void startThread();
	void update(int64_t ms = 0);

	//自定义协议重载doRequest
	virtual void onRequest(const Connection& conn, const char* data, uint32_t len);
	virtual void onClose(const Connection& conn, uint32_t closeType);
	virtual void onAccept(const Connection& conn) {}

	//RPC协议重载doRpcRequest
//	virtual int doRpcRequest(Connection::ptr conn, const RpcRequest& req, RpcResponse& resp) {
//		return 0;
//	}
private:
	void loop();

protected:
	SocketLoop* m_loop;		// loop对象
	StormListener* m_listener;	// 监听器
	std::thread m_thread;
};

}

#endif
