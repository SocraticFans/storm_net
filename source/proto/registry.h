/*代码生成器自动生成，请不要手动修改!*/
#ifndef _STORM_RPC_REGISTRY_H_
#define _STORM_RPC_REGISTRY_H_

#include "framework/storm_service.h"
#include "framework/storm_service_proxy.h"

#include "registry.pb.h"
using namespace storm;


class RegistryService : public storm::StormService {
public:
	RegistryService(storm::SocketLoop* loop, storm::StormListener* listener)
		:storm::StormService(loop, listener) {
	}
	virtual ~RegistryService(){}

	virtual int32_t onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp);

	virtual int32_t Query(const storm::Connection& conn, const QueryServiceReq& req, QueryServiceAck& ack) {return 0;}
	virtual int32_t HeartBeat(const storm::Connection& conn, const ServiceHeartBeatReq& req, ServiceHeartBeatAck& ack) {return 0;}
	virtual int32_t Stop(const storm::Connection& conn, const ServiceStopReq& req, ServiceStopAck& ack) {return 0;}
};

class RegistryServiceProxyCallBack : public storm::ServiceProxyCallBack {
public:
	virtual ~RegistryServiceProxyCallBack(){}

	virtual void dispatch(storm::RequestMessage* req);

	virtual void callback_Query(int32_t ret, const QueryServiceAck& ack) {
		throw std::runtime_error("no implement callback_Query");
	};
	virtual void callback_HeartBeat(int32_t ret, const ServiceHeartBeatAck& ack) {
		throw std::runtime_error("no implement callback_HeartBeat");
	};
	virtual void callback_Stop(int32_t ret, const ServiceStopAck& ack) {
		throw std::runtime_error("no implement callback_Stop");
	};
};

class RegistryServiceProxy : public storm::ServiceProxy {
public:
	virtual ~RegistryServiceProxy(){}

	RegistryServiceProxy* hash(uint64_t code) {
		storm::ServiceProxy::hash(code);
		return this;
	}

	int32_t Query(const QueryServiceReq& req, QueryServiceAck& ack);
	void async_Query(RegistryServiceProxyCallBack* cb, const QueryServiceReq& req, bool broadcast = false);

	int32_t HeartBeat(const ServiceHeartBeatReq& req, ServiceHeartBeatAck& ack);
	void async_HeartBeat(RegistryServiceProxyCallBack* cb, const ServiceHeartBeatReq& req, bool broadcast = false);

	int32_t Stop(const ServiceStopReq& req, ServiceStopAck& ack);
	void async_Stop(RegistryServiceProxyCallBack* cb, const ServiceStopReq& req, bool broadcast = false);
};

#endif
