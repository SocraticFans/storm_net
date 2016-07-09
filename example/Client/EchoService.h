/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_GAMESERVICE_H_
#define _STORM_RPC_GAMESERVICE_H_

#include "framework/storm_service_proxy.h"

#include "echo.pb.h"

using namespace storm;

class EchoServiceProxyCallBack : public ServiceProxyCallBack {
public:
	virtual ~EchoServiceProxyCallBack() {}

	virtual void dispatch(RequestMessage* message);

	virtual void callback_Echo(int32_t ret, const EchoAck& ack) {
		throw std::runtime_error("no implement callback_Echo");
	}
};

class EchoServiceProxy : public storm::ServiceProxy {
public:
	EchoServiceProxy() {}

	virtual ~EchoServiceProxy(){}

	int Echo(const EchoReq& req, EchoAck& ack);
	void async_Echo(EchoServiceProxyCallBack* cb, const EchoReq& req);
};

#endif
