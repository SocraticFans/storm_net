#ifndef _STORM_PROXY_MANAGER_H_
#define _STORM_PROXY_MANAGER_H_

#include "net/socket_loop.h"
#include "net/socket_handler.h"
#include "util/util_log.h"

#include "storm_service_proxy.h"

namespace storm {
class ServiceProxyManager {
public:
	typedef std::map<std::string, ServiceProxy*> ProxyMap;

	ServiceProxyManager(SocketLoop* loop)
		:m_loop(loop) {}

	~ServiceProxyManager();

	void updateInMainLoop();
	void updateInNetLoop();

	void pushMessage(RequestMessage* message) {
		m_messages.push_back(message);
	}


	// 获得一个service代理
	template <typename T>
	T* stringToProxy(const std::string& serviceName) {
		ScopeMutex<Mutex> lock(m_mutex);
		ProxyMap::iterator it = m_proxys.find(serviceName);
		if (it != m_proxys.end()) {
			return dynamic_cast<T*>(it->second);
		}
		T* proxy = new T();
		proxy->setLoop(m_loop);
		proxy->setManager(this);
		if (proxy->parseFromString(serviceName) == false) {
			delete proxy;
			STORM_ERROR << "error! string: " << serviceName;
			return NULL;
		}
		m_proxys.insert(std::make_pair(serviceName, proxy));
		return proxy;
	}

private:
	SocketLoop* m_loop;
	Mutex m_mutex;
	ProxyMap m_proxys;
	LockQueue<RequestMessage*> m_messages;		// 异步消息回调队列
};

}

#endif
