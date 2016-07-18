#include "RegistryServiceImp.h"
#include "ServiceManager.h"

namespace Registry {

int32_t RegistryServiceImp::Query(const Connection& conn, const QueryServiceReq& req, QueryServiceAck& ack) {
	vector<EndPoint> services;
	ServiceManager::instance()->getService(services, req.app_name(), req.server_name(), req.service_name(), req.set_name());
	for (auto it = services.begin(); it != services.end(); ++it) {
		const EndPoint& s = *it;
		if (!s.active) {
			continue;
		}
		ServiceProxyEndPointInfo* ep = ack.add_active_endpoints();
		ep->set_ip(s.ip);
		ep->set_port(s.port);
	}
	return 0;
}

int32_t RegistryServiceImp::HeartBeat(const storm::Connection& conn, const ServiceHeartBeatReq& req, ServiceHeartBeatAck& ack) {
	ServiceManager::instance()->heartBeat(req.info());
	return 0;
}

}
