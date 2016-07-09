#include "storm_service_proxy.h"
#include "storm_proxy_manager.h"

#include <vector>

#include "util/util_string.h"
#include "util/util_protocol.h"
#include "util/util_log.h"
#include "util/util_time.h"

namespace storm {

struct ProxyThreadData : public ThreadSingleton<ProxyThreadData> {
	ProxyThreadData():m_hashCode(-1) {}
	int64_t m_hashCode;
};

void ProxyEndPoint::onConnect(Socket* s) {
	ScopeMutex<Mutex> lock(m_mutex);
	m_connId = s->id;
	m_connected = true;
	if (m_buffer->getSize()) {
		m_loop->send(m_connId, m_buffer->getHead(), m_buffer->getSize());
		m_buffer->reset();
	}
}

// onClose 不加锁了，单线程模式下send中可能连接断开，调到onClose造成死锁
void ProxyEndPoint::onClose(Socket* s, uint32_t closeType) {
	ScopeMutex<Mutex> lock(m_mutex);
	if (m_connId != s->id) {
		return ;
	}
	lock.unlock();

	m_proxy->onClose(this, closeType);

	lock.lock();
	m_connId = -1;
	m_connected = false;
	lock.unlock();
}

void ProxyEndPoint::onPacket(Socket* s, const char* data, uint32_t len) {
	m_proxy->onPacket(this, data, len);
}

void ProxyEndPoint::send(const char* data, uint32_t len) {
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

void ServiceProxy::onClose(ProxyEndPoint* ep, uint32_t closeType) {
	STORM_INFO << "onClose! id: " << ep->m_connId << ", closeType: " << etos((SocketCloseType)closeType);

	if (closeType == CloseType_ConnectFail) {
		// 非指定ip访问的，移到非活跃ep里面
		//delEndPoint(pack->id);
	}

	std::vector<RequestMessage*> affectMsg;
	ScopeMutex<Mutex> lock(m_messMutex);
	for (std::map<uint32_t, RequestMessage*>::iterator it = m_reqMessages.begin(); it != m_reqMessages.end(); ) {
		if (it->second->ep != ep) {
			++it;
			continue;
		}
		//m_timeout.del(it->first);
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
		} else if (closeType == CloseType_ConnectFail) {
			message->status = ResponseStatus_NetError;
		}
		finishInvoke(message);
	}
}

void ServiceProxy::onPacket(ProxyEndPoint* ep, const char* data, uint32_t len) {
	RpcResponse* resp = new RpcResponse();
	if (!resp->ParseFromArray(data, len)) {
		STORM_ERROR << "parse response error";
		delete resp;
		return;
	}

	RequestMessage* message = getAndDelReqMessage(resp->request_id());
	if (message == NULL) {
		STORM_ERROR << "error, unknown requestId";
		delete resp;
		return;
	}

	message->resp = resp;
	message->status = ResponseStatus_Ok;
	finishInvoke(message);
}

bool ServiceProxy::parseFromString(const string& config) {
	string::size_type pos = config.find('@');
	if (pos != string::npos) {
		m_needLocator = false;	
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
			m_endPoints.push_back(ep);
		}
	} else {
		m_needLocator = true;
		//TODO 同步调用locator
	}

	return true;
}

void ServiceProxy::hash(int64_t code) {
	ProxyThreadData::instance()->m_hashCode = code;
}

RequestMessage* ServiceProxy::newRequest(InvokeType type, ServiceProxyCallBack* cb) {
	RequestMessage* message = new RequestMessage;

	message->cb = cb;
	if (type == InvokeType_Async && cb == NULL) {
		type = InvokeType_OneWay;
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
	IoBuffer* buffer = PacketProtocolLen::encode(message->req);


	ProxyEndPoint* ep = selectEndPoint();
	if (ep == NULL) {
		delete buffer;
		STORM_ERROR << "no endpoint ";
		message->status = ResponseStatus_NetError;
		finishInvoke(message);
		return;
	}
	message->ep = ep;

	// 不是单向调用的都保存请求
	if (invokeType != InvokeType_OneWay) {
		saveMessage(requestId, message);
	}

	ep->send(buffer->getHead(), buffer->getSize());
	delete buffer;

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
		//STORM_DEBUG << "finishInvoke";
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
	if (m_endPoints.empty()) {
		return NULL;
	}
	int64_t code = ProxyThreadData::instance()->m_hashCode;
	if (code != -1) {
		return m_endPoints[code % m_endPoints.size()];
	}
	return m_endPoints[m_sequeue % m_endPoints.size()];
}

void ServiceProxy::saveMessage(uint32_t requestId, RequestMessage* message) {
	ScopeMutex<Mutex> lock(m_messMutex);
	m_reqMessages[requestId] = message;
//	uint32_t now = UtilTime::getNow();
//	m_timeout.add(requestId, now);
}

RequestMessage* ServiceProxy::getAndDelReqMessage(uint32_t requestId) {
	ScopeMutex<Mutex> lock(m_messMutex);
	MessageMap::iterator it = m_reqMessages.find(requestId);
	if (it == m_reqMessages.end()) {
		return NULL;
	}
	RequestMessage* temp = it->second;
	m_reqMessages.erase(it);
	//m_timeout.del(requestId);
	return temp;
}

}
