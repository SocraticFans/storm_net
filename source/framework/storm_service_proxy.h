#ifndef _STORM_SERVICE_PROXY_H_
#define _STORM_SERVICE_PROXY_H_

#include "net/socket_loop.h"
#include "net/socket_handler.h"

#include "util/util_misc.h"
#include "util/util_thread.h"
#include "util/util_timelist.h"
#include "util/util_singleton.h"

#include "connection.h"
#include "server_config.h"

namespace storm {
class StormProxyManager;

class StormServiceProxy {
public:
	StormServiceProxy(SocketLoop* loop) {
	}
	virtual ~StormServiceProxy() {}

	// 继承类实现一个hash方法，return this
	void hash(uint64_t code);

private:
};

/*
class ServiceProxy : public SocketProxy {
public:
	typedef std::shared_ptr<ServiceProxy> ptr;
	ServiceProxy(SocketConnector* connector);
	bool parseFromString(const string& config);
	virtual ~ServiceProxy(){}

	virtual void onConnect(uint32_t id) {
		STORM_DEBUG << "onConnect " << id;
	}
	virtual void onData(uint32_t id, IOBuffer::ptr buffer);
	virtual void onClose(uint32_t id, uint32_t closeType);
	virtual void doRequest(RecvPacket::ptr pack);
	virtual void doClose(RecvPacket::ptr pack);

	void terminate();

	void doInvoke(ReqMessage* req);
	void finishInvoke(ReqMessage* req);

	void doTimeOut();
	void doTimeClose(uint32_t id);

	uint32_t selectEndPoint(uint32_t requestId);
	void updateEndPoints();

private:
	void setReqMessage(uint32_t requestId, ReqMessage* mess);
	ReqMessage* getAndDelReqMessage(uint32_t requestId);

	void setEndPoints(vector<uint32_t>& eps);
	void delEndPoint(uint32_t id);

protected:
	SocketConnector* m_connector;
	bool m_needLocator;
	string m_objName;

	ProtocolType	m_protocol;

	Mutex m_epMutex;
	vector<uint32_t> m_endPoints;

	//key: service name, ip, port
	map<string, uint32_t> m_clients;

	//发出去的消息
	map<uint32_t, ReqMessage*> m_reqMessages;
	Mutex m_messMutex;
	atomic_uint m_sequeue;

	TimeList<uint32_t, uint32_t> m_timeout;
};
*/

}

#endif
