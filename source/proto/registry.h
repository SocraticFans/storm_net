/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_REGISTRY_H_
#define _STORM_RPC_REGISTRY_H_

#include "framework/storm_service.h"
#include "framework/storm_service_proxy.h"

#include "registry.pb.h"

using namespace storm;


class RegistryService : public StormService {
public:
	RegistryService(SocketLoop* loop, StormListener* listener)
		:StormService(loop, listener) {
	}
	virtual ~RegistryService(){}

	virtual int32_t onRpcRequest(const Connection& conn, const RpcRequest& req, RpcResponse& resp);

	virtual void Query(const Connection& conn, const QueryServiceReq& req, QueryServiceAck& ack) {}
};

class RegistryServiceProxyCallBack : public ServiceProxyCallBack {
public:
	virtual ~RegistryServiceProxyCallBack(){}

	virtual void dispatch(RequestMessage* req);

	virtual void callback_Query(int32_t ret, const QueryServiceAck& ack) {
		throw std::runtime_error("no implement callback_Query");
	};
};

class RegistryServiceProxy : public ServiceProxy {
public:
	virtual ~RegistryServiceProxy(){}

	int32_t Query(const QueryServiceReq& req, QueryServiceAck& ack);
	void async_Query(RegistryServiceProxyCallBack* cb, const QueryServiceReq& req, bool broadcast = false);
};

#endif
