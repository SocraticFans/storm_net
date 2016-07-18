#include "storm_service_proxy.h"
#include "storm_proxy_manager.h"

#include <vector>

#include "util/util_string.h"
#include "util/util_protocol.h"
#include "util/util_log.h"
#include "util/util_time.h"
#include "proto/registry.h"

namespace storm {

struct ProxyThreadData : public ThreadSingleton<ProxyThreadData> {
	ProxyThreadData():m_hashCode(-1) {}
	int64_t m_hashCode;
};

void ProxyEndPoint::onConnect(Socket* s) {
	//STORM_INFO;
	ScopeMutex<Mutex> lock(m_mutex);
//	m_connId = s->id;
	m_connected = true;
	if (m_buffer->getSize()) {
		m_loop->send(m_connId, m_buffer->getHead(), m_buffer->getSize());
		m_buffer->reset();
	}
}

void ProxyEndPoint::onClose(Socket* s, uint32_t closeType) {
	ScopeMutex<Mutex> lock(m_mutex);
	if (m_connId != s->id) {
		return ;
	}
	m_connId = -1;
	m_connected = false;
	m_buffer->reset();
	lock.unlock();

	m_proxy->onClose(this, closeType);
}

void ProxyEndPoint::onPacket(Socket* s, const char* data, uint32_t len) {
	m_proxy->onPacket(this, data, len);
}

void ProxyEndPoint::send(const char* data, uint32_t len) {
	//STORM_INFO << m_connected << m_connId;
	ScopeMutex<Mutex> lock(m_mutex);
	if (m_connected) {
		// 已经连接, 直接发送
		m_loop->send(m_connId, data, len);
	} else if (m_connId != -1) {
		// 连接中，加入发送缓冲区
		m_buffer->push_back(data, len);
	} else {
		// 还没有连接，则连接，同时将数据加入发送缓冲区
		m_buffer->push_back(data, len);
		m_connId = m_loop->connect(this, m_ip, m_port);
	}
}

void ProxyEndPoint::send(const std::string& data) {
	send(data.c_str(), data.size());
}

ServiceProxy::ServiceProxy()
:m_loop(NULL)
,m_mgr(NULL)
,m_inLoop(false)
,m_sequeue(0) {
	m_timeout.setTimeout(100000);
	m_timeout.setFunction(std::bind(&ServiceProxy::doReqTimeOut, this, std::placeholders::_1));
}

ServiceProxy::~ServiceProxy() {
	for (auto it = m_activeEndPoints.begin(); it != m_activeEndPoints.end(); ++it) {
		delete *it;
	}
	for (auto it = m_inactiveEndPoints.begin(); it != m_inactiveEndPoints.end(); ++it) {
		delete *it;
	}
	for (auto it = m_reqMessages.begin(); it != m_reqMessages.end(); ++it) {
		delete it->second;
	}
}

void ServiceProxy::doTimeOut() {
	m_timeout.timeout(UtilTime::getNowMS());
}

void ServiceProxy::doReqTimeOut(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_messMutex);
	MessageMap::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return ;
	}
	RequestMessage* message = it->second;
	m_reqMessages.erase(it);
	lock.unlock();

	message->status = ResponseStatus_TimeOut;
	finishInvoke(message);
}

void ServiceProxy::onClose(ProxyEndPoint* ep, uint32_t closeType) {
	//STORM_INFO << m_name << ":" << ep->m_ip << ":" << ep->m_port << ", closeType: " << etos((SocketCloseType)closeType);

	if (closeType == CloseType_ConnectFail || closeType == CloseType_ConnTimeOut) {
		// 非指定ip访问的，移到非活跃ep里面
		if (m_needLocator) {
			moveToInactive(ep);
		}
	}

	std::vector<RequestMessage*> affectMsg;
	ScopeMutex<Mutex> lock(m_messMutex);
	for (std::map<uint32_t, RequestMessage*>::iterator it = m_reqMessages.begin(); it != m_reqMessages.end(); ) {
		if (it->second->ep != ep) {
			++it;
			continue;
		}
		RequestMessage* message = it->second;
		affectMsg.push_back(message);
		m_reqMessages.erase(it++);

	}
	lock.unlock();

	for (auto it = affectMsg.begin(); it != affectMsg.end(); ++it) {
		RequestMessage* message = *it;
		message->status = ResponseStatus_Error;
		if (closeType == CloseType_Timeout) {
			message->status = ResponseStatus_TimeOut;
		} else if (closeType == CloseType_ConnectFail || closeType == CloseType_ConnTimeOut) {
			message->status = ResponseStatus_NetError;
		}
		finishInvoke(message);
		m_timeout.del(message->requestId);
	}
}

void ServiceProxy::onPacket(ProxyEndPoint* ep, const char* data, uint32_t len) {
	RpcResponse* resp = new RpcResponse();
	if (!resp->ParseFromArray(data, len)) {
		STORM_ERROR << m_name << ", parse response error";
		delete resp;
		return;
	}

	RequestMessage* message = getAndDelReqMessage(resp->request_id());
	if (message == NULL) {
		STORM_ERROR << m_name << ", error, unknown requestId: " << resp->request_id();
		delete resp;
		return;
	}
	m_timeout.del(resp->request_id());

	message->resp = resp;
	message->status = ResponseStatus_Ok;
	finishInvoke(message);
}

bool ServiceProxy::parseFromString(const string& config, const string& setName) {
	m_setName = setName;
	string::size_type pos = config.find('@');
	if (pos != string::npos) {
		m_needLocator = false;	
		m_name = config.substr(0, pos) + "Proxy";
		
		vector<string> endpoints = UtilString::splitString(config.substr(pos + 1), ":");
		for (uint32_t i = 0; i < endpoints.size(); ++i) {
			string desc = endpoints[i];
			vector<string> v = UtilString::splitString(desc, " \t\r\n");
			if (v.size() < 3 || v.size() % 2 != 1) {
				return false;
			}
			//只支持tcp
			if (v[0] != "tcp") {
				return false;
			}

			string host;
			uint32_t port = 0;
			for (unsigned j = 1; j < v.size(); j += 2) {
				const string &opt = v[j];
				const string &arg = v[j + 1];
				if (opt.length() != 2 || opt[0] != '-') {
					return false;
				}
				if (opt[1] == 'h') {
					host = arg;
				} else if (opt[1] == 'p') {
					port = UtilString::strto<uint32_t>(arg);
				} else if (opt[1] == 't') {

				} else {
					continue;
				}
			}
			if (host.empty() || port == 0) {
				return false;
			}
			ProxyEndPoint* ep = new ProxyEndPoint(m_loop, this);
			ep->m_ip = host;
			ep->m_port = port;
			m_activeEndPoints.push_back(ep);
		}
	} else {
		m_needLocator = true;
		m_name = config + "Proxy";
		vector<string> serviceNames = UtilString::splitString(config, ".");
		if (serviceNames.size() < 3) {
			STORM_ERROR << "service name shoud be [appName.serverName.serviceName]";
			return false;
		}
		m_appName = serviceNames[0];
		m_serverName = serviceNames[1];
		m_serviceName = serviceNames[2];
		// 同步调用locator
		QueryServiceReq req;
		req.set_app_name(m_appName);
		req.set_server_name(m_serverName);
		req.set_service_name(m_serviceName);
		req.set_set_name(m_setName);
		QueryServiceAck ack;
		int32_t ret = m_mgr->getRegistryProxy()->Query(req, ack);
		if (ret) {
			STORM_ERROR << "updateEndPoints Error: " << ret;
		} else {
			updateEndPoints(ack);
		}
	}

	return true;
}

class RegistryCallBack : public RegistryServiceProxyCallBack {
public:
	RegistryCallBack(ServiceProxy* proxy)
		:m_proxy(proxy) {}

	virtual void callback_Query(int32_t ret, const QueryServiceAck& ack) {
		if (ret) {
			STORM_ERROR << "updateEndPoints Error: " << ret;
		} else {
			m_proxy->updateEndPoints(ack);
		}
	};

private:
	ServiceProxy* m_proxy;
};

void ServiceProxy::doAsyncUpdateEndPoints() {
	QueryServiceReq req;
	req.set_app_name(m_appName);
	req.set_server_name(m_serverName);
	req.set_service_name(m_serviceName);
	req.set_set_name(m_setName);

	m_mgr->getRegistryProxy()->async_Query(new RegistryCallBack(this), req);
}

void ServiceProxy::hash(int64_t code) {
	ProxyThreadData::instance()->m_hashCode = code;
}

RequestMessage* ServiceProxy::newRequest(InvokeType type, ServiceProxyCallBack* cb, bool broadcast) {
	RequestMessage* message = new RequestMessage;

	message->cb = cb;
	if (type == InvokeType_Async && cb == NULL) {
		type = InvokeType_OneWay;
		message->broadcast = broadcast;
	}
	message->invokeType = type;
	message->req.set_invoke_type(type);
	message->threadId = getTid();

	return message;
}

void ServiceProxy::delRequest(RequestMessage* message) {
	if (message->cb) {
		delete message->cb;
		message->cb = NULL;
	}
	if (message->resp) {
		delete message->resp;
		message->resp = NULL;
	}
	delete message;
}

void ServiceProxy::doInvoke(RequestMessage* message) {
	//异步请求message可能被删掉了,所以这里把invokeType复制
	uint32_t invokeType = message->invokeType;
	uint32_t requestId = m_sequeue.fetch_add(1);
	message->req.set_request_id(requestId);
	message->requestId = requestId;
	IoBuffer* buffer = PacketProtocolLen::encode(message->req);


	if (invokeType == InvokeType_OneWay && message->broadcast) {
		ScopeMutex<Mutex> lock(m_epMutex);
		for (auto it = m_activeEndPoints.begin(); it != m_activeEndPoints.end(); ++it) {
			(*it)->send(buffer->getHead(), buffer->getSize());
		}
	} else {
		ProxyEndPoint* ep = selectEndPoint();
		if (ep == NULL) {
			delete buffer;
			STORM_ERROR << m_name << ", no endpoint ";
			message->status = ResponseStatus_NetError;
			finishInvoke(message);
			return;
		}
		message->ep = ep;
		ep->send(buffer->getHead(), buffer->getSize());
	}
	delete buffer;

	// 不是单向调用的都保存请求
	if (invokeType != InvokeType_OneWay) {
		saveMessage(requestId, message);
		m_timeout.add(requestId, UtilTime::getNowMS());
	}

	// 同步等待
	if (invokeType == InvokeType_Sync) {
		ScopeMutex<Notifier> lock(message->notifier);
		if (message->back == false) {
			message->notifier.wait();
		}
	}
}

// 1.同步请求只需唤醒调用线程 2.异步请求调用回调函数
void ServiceProxy::finishInvoke(RequestMessage* message) {
	if (message->invokeType == InvokeType_Sync) {
		ScopeMutex<Notifier> lock(message->notifier);
		message->back = true;
		message->notifier.signal();
		return;
	} else if (message->invokeType == InvokeType_Async) {
		if (getTid() == message->threadId) {
			message->cb->dispatch(message);
		} else {
			m_mgr->pushMessage(message);
		}
	}
}

ProxyEndPoint* ServiceProxy::selectEndPoint() {
	ScopeMutex<Mutex> lock(m_epMutex);
	if (m_activeEndPoints.empty()) {
		return NULL;
	}
	int64_t code = ProxyThreadData::instance()->m_hashCode;
	ProxyThreadData::instance()->m_hashCode = -1;
	if (code != -1) {
		return m_activeEndPoints[code % m_activeEndPoints.size()];
	}
	return m_activeEndPoints[m_sequeue % m_activeEndPoints.size()];
}

void ServiceProxy::saveMessage(uint32_t requestId, RequestMessage* message) {
	ScopeMutex<Mutex> lock(m_messMutex);
	m_reqMessages[requestId] = message;
}

RequestMessage* ServiceProxy::getAndDelReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_messMutex);
	MessageMap::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return NULL;
	}
	RequestMessage* temp = it->second;
	m_reqMessages.erase(it);
	return temp;
}

void ServiceProxy::moveToInactive(ProxyEndPoint* ep) {
	ScopeMutex<Mutex> lock(m_epMutex);
	for (auto it = m_activeEndPoints.begin(); it != m_activeEndPoints.end(); ++it) {
		if (*it == ep) {
			m_activeEndPoints.erase(it);
			break;
		}
	}
	for (auto it = m_inactiveEndPoints.begin(); it != m_inactiveEndPoints.end(); ) {
		if (*it == ep) {
			it = m_activeEndPoints.erase(it);
		} else {
			++it;
		}
	}
	m_inactiveEndPoints.push_back(ep);
}

void ServiceProxy::updateEndPoints(const QueryServiceAck& ack) {
	ScopeMutex<Mutex> lock(m_epMutex);
	std::list<ProxyEndPoint*> all;
	for (auto it = m_activeEndPoints.begin(); it != m_activeEndPoints.end(); ++it) {
		all.push_back(*it);
	}
	for (auto it = m_inactiveEndPoints.begin(); it != m_inactiveEndPoints.end(); ++it) {
		all.push_back(*it);
	}
	m_activeEndPoints.clear();
	m_inactiveEndPoints.clear();

	for (int32_t i = 0; i < ack.active_endpoints_size(); ++i) {
		std::string ip = ack.active_endpoints(i).ip();
		uint32_t port = ack.active_endpoints(i).port();
		bool found = false;
		for (auto it = all.begin(); it != all.end(); ) {
			ProxyEndPoint* ep = *it;
			if (ep->m_ip == ip && ep->m_port == port) {
				found = true;
				m_activeEndPoints.push_back(ep);
				all.erase(it++);
				break;
			} else {
				it++;
			}
		}
		if (!found) {
			ProxyEndPoint* ep = new ProxyEndPoint(m_loop, this);
			ep->m_ip = ip;
			ep->m_port = port;
			m_activeEndPoints.push_back(ep);
		}
	}

	// 其他的暂时全放到不活跃列表里
	for (auto it = all.begin(); it != all.end(); ++it) {
		m_inactiveEndPoints.push_back(*it);
	}
}

}
