#ifndef STORM_SERVICE_H_
#define STORM_SERVICE_H_

#include <thread>

#include "net/socket_loop.h"
#include "util/util_thread.h"
#include "proto/rpc_proto.pb.h"

#include "connection.h"

namespace storm {
class StormListener;

// Service业务代码基类
class StormService {
public:
	StormService(SocketLoop* loop, StormListener* listener);
	virtual ~StormService(){}

	// 初始化
	virtual void init() {}
	// 销毁
	virtual void destroy() {}

	//自定义协议重载onRequest
	virtual void onRequest(const Connection& conn, const char* data, uint32_t len);
	virtual void onClose(const Connection& conn, uint32_t closeType) {}
	virtual void onAccept(const Connection& conn) {}

	//RPC协议重载onRpcRequest
	virtual int onRpcRequest(const Connection& conn, const RpcRequest& req, RpcResponse& resp) {
		return 0;
	}

	void startThread();
	void terminateThread();
	void update(int64_t ms = 0);

private:
	void loop();

protected:
	SocketLoop* m_loop;			// loop对象
	StormListener* m_listener;	// 监听器
	std::thread m_thread;		// 独立业务线程
	bool m_running;				// 独立线程运行标记
};

}

#endif
