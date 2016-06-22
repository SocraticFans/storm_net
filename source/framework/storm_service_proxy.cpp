#include "storm_service_proxy.h"

namespace storm {

struct ProxyThreadData : public ThreadSingleton<ProxyThreadData> {
	uint32_t m_hashCode;
};

void StormServiceProxy::hash(uint64_t code) {
	ProxyThreadData::instance()->m_hashCode = code;
}

}
