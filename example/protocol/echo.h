/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_ECHO_H_
#define _STORM_RPC_ECHO_H_

#include "framework/storm_service.h"
#include "framework/storm_service_proxy.h"

#include "echo.pb.h"

using namespace storm;


class EchoService : public StormService {
public:
	EchoService(SocketLoop* loop, StormListener* listener)
		:StormService(loop, listener) {
	}
	virtual ~EchoService(){}

	virtual int32_t onRpcRequest(const Connection& conn, const RpcRequest& req, RpcResponse& resp);

	virtual void Echo(const Connection& conn, const EchoReq& req, EchoAck& ack) {}
};

class EchoServiceProxyCallBack : public ServiceProxyCallBack {
public:
	virtual ~EchoServiceProxyCallBack(){}

	virtual void dispatch(RequestMessage* req);

	virtual void callback_Echo(int32_t ret, const EchoAck& ack) {
		throw std::runtime_error("no implement callback_Echo");
	};
};

class EchoServiceProxy : public ServiceProxy {
public:
	virtual ~EchoServiceProxy(){}

	int32_t Echo(const EchoReq& req, EchoAck& ack);
	void async_Echo(EchoServiceProxyCallBack* cb, const EchoReq& req, bool broadcast = false);
};

#endif
