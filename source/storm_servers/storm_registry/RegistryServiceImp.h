#ifndef _STORM_REGISTRY_SERVICE_IMP_H_
#define _STORM_REGISTRY_SERVICE_IMP_H_
#include "proto/registry.h"

using namespace storm;

namespace Registry {
class RegistryServiceImp : public RegistryService {
public:
	RegistryServiceImp(SocketLoop* loop, StormListener* listener)
		:RegistryService(loop, listener) {}
	virtual ~RegistryServiceImp() {}

	virtual void Query(const Connection& conn, const QueryServiceReq& req, QueryServiceAck& ack);
};

}

#endif
