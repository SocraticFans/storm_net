#include "storm_proxy_manager.h"

namespace storm {

ServiceProxyManager::~ServiceProxyManager() {
	for (ProxyMap::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		delete it->second;
	}
}

void ServiceProxyManager::updateInMainLoop() {
	std::list<RequestMessage*> messages;
	m_messages.swap(messages);

	for (auto it = messages.begin(); it != messages.end(); ++it) {
		RequestMessage* message = *it;
		message->cb->dispatch(message);
	}
}

void ServiceProxyManager::updateInNetLoop() {
	ScopeMutex<Mutex> lock(m_mutex);
	for (ProxyMap::iterator it = m_proxys.begin(); it != m_proxys.end(); ++it) {
		it->second->doTimeOut();
	}
}

}
