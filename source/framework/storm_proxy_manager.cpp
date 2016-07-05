#include "storm_proxy_manager.h"

namespace storm {

ServiceProxyManager::~ServiceProxyManager() {
	for (ProxyMap::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		delete it->second;
	}
}
}
