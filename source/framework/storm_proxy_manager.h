#ifndef _STORM_PROXY_MANAGER_H_
#define _STORM_PROXY_MANAGER_H_

#include "net/socket_loop.h"
#include "net/socket_handler.h"
#include "util/util_log.h"

#include "storm_service_proxy.h"
#include "proto/registry.h"

namespace storm {
class ServiceProxyManager {
public:
	typedef std::map<std::string, ServiceProxy*> ProxyMap;

	ServiceProxyManager(SocketLoop* loop);
	~ServiceProxyManager();

	void updateInMainLoop();
	void updateInNetLoop();

	void pushMessage(RequestMessage* message) {
		m_messages.push_back(message);
	}

	void setRegistryProxy(RegistryServiceProxy* proxy) {
		m_registryProxy = proxy;
	}

	RegistryServiceProxy* getRegistryProxy() {
		return m_registryProxy;
	}

	void setUpdateTime(uint32_t time) {
		m_updateTime = time;
	}

	// 获得一个service代理
	template <typename T>
	T* stringToProxy(const std::string& serviceName, const std::string& setName = std::string("")) {
		ScopeMutex<Mutex> lock(m_mutex);
		ProxyMap::iterator it = m_proxys.find(serviceName);
		if (it != m_proxys.end()) {
			return dynamic_cast<T*>(it->second);
		}
		T* proxy = new T();
		proxy->setLoop(m_loop);
		proxy->setManager(this);
		m_proxys.insert(std::make_pair(serviceName, proxy));
		lock.unlock();

		if (proxy->parseFromString(serviceName, setName) == false) {
			delete proxy;
			STORM_ERROR << "error! string: " << serviceName;
			// 删了
			lock.lock();
			m_proxys.erase(serviceName);
			lock.unlock();
			return NULL;
		}
		return proxy;
	}

private:
	SocketLoop* m_loop;
	Mutex m_mutex;
	ProxyMap m_proxys;
	LockQueue<RequestMessage*> m_messages;		// 异步消息回调队列
	RegistryServiceProxy* m_registryProxy;		// registryProxy
	uint32_t m_lastUpdateTime;					// 上次更新时间
	uint32_t m_updateTime;						// 更新间隔
};

}

#endif
