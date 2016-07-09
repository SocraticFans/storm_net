#ifndef _STORM_SERVICE_PROXY_H_
#define _STORM_SERVICE_PROXY_H_

#include <atomic>

#include "net/socket_loop.h"
#include "net/socket_handler.h"

#include "util/util_misc.h"
#include "util/util_thread.h"
#include "util/util_timelist.h"
#include "util/util_singleton.h"
#include "util/util_log.h"

#include "connection.h"
#include "server_config.h"
#include "storm_request.h"

namespace storm {
class ServiceProxy;
class ServiceProxyManager;

class ServiceProxyCallBack {
public:
	virtual ~ServiceProxyCallBack() {}
	virtual void dispatch(RequestMessage* message) = 0;
};

class ProxyEndPoint : public SocketHandler {
public:
	friend class ServiceProxy;

	ProxyEndPoint(SocketLoop* loop, ServiceProxy* proxy)
	:SocketHandler(loop)
	,m_proxy(proxy)
	,m_connId(-1)
	,m_connected(false) {
		m_buffer = new IoBuffer(4096);
	}

	virtual ~ProxyEndPoint() {
		delete m_buffer;
	}

	virtual void onConnect(Socket* s);
	virtual void onClose(Socket* s, uint32_t closeType);
	virtual void onPacket(Socket* s, const char* data, uint32_t len);

	void send(const char* data, uint32_t len);
	void send(const std::string& data);
	

private:
	ServiceProxy* m_proxy;				// Service代理
	Mutex m_mutex;						// 锁
	int32_t m_connId;					// 连接id
	bool m_connected;					// 是否已经连接
	std::string m_ip;					// ip
	uint32_t m_port;					// port
	IoBuffer* m_buffer;					// 发送缓冲
};

class ServiceProxy {
public:
	ServiceProxy();
	virtual ~ServiceProxy();

	void setLoop(SocketLoop* loop) { m_loop = loop; }
	void setManager(ServiceProxyManager* mgr) { m_mgr = mgr; }

	bool parseFromString(const std::string& config);

	// 继承类实现一个hash方法，return this
	void hash(int64_t code);

	void onClose(ProxyEndPoint* ed, uint32_t closeType);
	void onPacket(ProxyEndPoint* ed, const char* data, uint32_t len);

	void doTimeOut();
	void doReqTimeOut(uint32_t requestId);

	static RequestMessage* newRequest(InvokeType type, ServiceProxyCallBack* cb = NULL, bool broadcast = false);
	static void delRequest(RequestMessage* message);

	// 调用入口
	// 把message发送出去
	void doInvoke(RequestMessage* message);

	// 调用结束的处理
	void finishInvoke(RequestMessage* message);

private:
	// 选择一个端点
	ProxyEndPoint* selectEndPoint();

	// 保存请求
	void saveMessage(uint32_t requestId, RequestMessage* message);

	// 获得并从map删除请求
	RequestMessage* getAndDelReqMessage(uint32_t requestId);

	// 移到不活跃中去
	void moveToInactive(ProxyEndPoint* ep);

private:
	typedef std::vector<ProxyEndPoint*> EndPointVec;
	typedef std::map<uint32_t, RequestMessage*> MessageMap;

	SocketLoop* m_loop;								// 网络循环
	ServiceProxyManager* m_mgr;						// proxy管理器
	bool m_inLoop;									// 消息是否在网络loop线程处理
	bool m_needLocator;								// 是否需要定位
	std::string m_name;

	EndPointVec m_activeEndPoints;					// 活跃的端点
	EndPointVec m_inactiveEndPoints;				// 故障的端点
	Mutex m_epMutex;								// endpoint锁

	atomic_uint m_sequeue;							// 请求sequeue
	Mutex m_messMutex;								// 请求消息锁
	MessageMap m_reqMessages; 						// 发出去的消息

	TimeList<uint32_t, uint64_t> m_timeout;			// 响应超时队列
};

template <typename T>
inline int32_t decodeResponse(RequestMessage* message, T& response) {
	int32_t ret = 0;
	do {
		if (message->status != 0) {
			ret = message->status;
			break;
		}
		RpcResponse* resp = message->resp;
		if (resp->ret() != 0) {
			ret = resp->ret();
			break;
		}
		if (!response.ParseFromString(resp->response())) {
			ret = ResponseStatus_CoderError;
			break;
		}

	} while (0);
	if (ret) {
		STORM_ERROR << "resp status: " << ret;
	}
	return ret;
}
}

#endif
