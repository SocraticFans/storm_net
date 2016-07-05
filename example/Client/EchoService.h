/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_GAMESERVICE_H_
#define _STORM_RPC_GAMESERVICE_H_

#include "framework/storm_service_proxy.h"

#include "echo.pb.h"

using namespace storm;
class EchoServiceProxy : public storm::ServiceProxy {
public:
	EchoServiceProxy(SocketLoop* loop)
		:ServiceProxy(loop) {}

	virtual ~EchoServiceProxy(){}

	int Echo(const EchoReq& req, EchoAck& ack);
	void async_Echo(ServiceProxyCallBack* cb, const EchoReq& req);
};

#endif
