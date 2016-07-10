#include "RegistryServiceImp.h"
#include "ServiceManager.h"

namespace Registry {

void RegistryServiceImp::Query(const Connection& conn, const QueryServiceReq& req, QueryServiceAck& ack) {
	vector<EndPoint> services;
	ServiceManager::instance()->getService(services, req.app_name(), req.server_name(), req.service_name(), req.set_name());
	for (auto it = services.begin(); it != services.end(); ++it) {
		const EndPoint& s = *it;
		ServiceProxyEndPointInfo* ep = ack.add_active_endpoints();
		ep->set_ip(s.ip);
		ep->set_port(s.port);
	}
}

}
