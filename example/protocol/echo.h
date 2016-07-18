/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_ECHO_H_
#define _STORM_RPC_ECHO_H_

#include "framework/storm_service.h"
#include "framework/storm_service_proxy.h"

#include "echo.pb.h"


class EchoService : public storm::StormService {
public:
	EchoService(storm::SocketLoop* loop, storm::StormListener* listener)
		:storm::StormService(loop, listener) {
	}
	virtual ~EchoService(){}

	virtual int32_t onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp);

	virtual int32_t Echo(const storm::Connection& conn, const EchoReq& req, EchoAck& ack) {return 0;}
};

class EchoServiceProxyCallBack : public storm::ServiceProxyCallBack {
public:
	virtual ~EchoServiceProxyCallBack(){}

	virtual void dispatch(storm::RequestMessage* req);

	virtual void callback_Echo(int32_t ret, const EchoAck& ack) {
		throw std::runtime_error("no implement callback_Echo");
	};
};

class EchoServiceProxy : public storm::ServiceProxy {
public:
	virtual ~EchoServiceProxy(){}

	EchoServiceProxy* hash(uint64_t code) {
		storm::ServiceProxy::hash(code);
		return this;
	}

	int32_t Echo(const EchoReq& req, EchoAck& ack);
	void async_Echo(EchoServiceProxyCallBack* cb, const EchoReq& req, bool broadcast = false);
};

#endif
