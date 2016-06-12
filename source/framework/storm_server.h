#ifndef STORM_SERVER_H_
#define STORM_SERVER_H_

#include <map>
#include <thread>

#include "net/socket_loop.h"

#include "util/util_config.h"
#include "util/util_string.h"

#include "server_config.h"
#include "storm_service.h"
#include "storm_listener.h"

namespace storm {

class StormServer {
public:
	StormServer();
	virtual ~StormServer();

	int run(int argc, char** argv);
	void terminate();

	virtual bool init() = 0;
	virtual void destroy() = 0;
	virtual void loop() = 0;

	void setServerType(ServerType type);

protected:
	// 开始服务监听，并设置网络loop是否直接处理网络包逻辑
	bool startService(bool inLoop);

	// 开启服务自己的逻辑处理线程，只在MultiThreadServer用到
	void startServiceThread();

	// 开启网络线程
	void startNetThread();

	// 网络loop
	void netLoop();

	template <class T>
	bool addService(const std::string& name);
	template <class T>
	void addService(const ServiceConfig& cfg);

	void setPacketParser(const std::string& name, SocketListener::PacketParser parser);

	void parseConfig(int argc, char** argv);
	void parseServerConfig(const CConfig& cfg);
	void parseClientConfig(const CConfig& cfg);

protected:
	typedef std::map<std::string, StormListener*> ListenerMapType;
	typedef std::vector<StormService*> ServiceVector;

	ServerConfig 	m_serverCfg;
	ClientConfig 	m_clientCfg;
	SocketLoop*		m_netLoop;				// 网络loop实例
	ListenerMapType m_listeners;			// 所有的监听器
	ServiceVector	m_inLoopServices;		// 在loop中处理逻辑的Service
	ServiceVector	m_notInLoopServices;	// 不在loop中处理逻辑的Service
	std::thread 	m_netThread;			// 网络线程
};

template <class T>
void StormServer::addService(const ServiceConfig& cfg) {
	//const std::string& key = cfg.name + cfg.ip + UtilString::tostr(cfg.port);
	const std::string& key = cfg.name;

	// 创建监听器
	ListenerMapType::iterator it = m_listeners.find(key);
	if (it == m_listeners.end()) {
		StormListener* l = new StormListener(m_netLoop);
		l->setName(cfg.name);
		l->setIp(cfg.ip);
		l->setPort(cfg.port);
		it = m_listeners.insert(std::make_pair(key, l)).first;
	}

	// 创建服务对象
	T* service = new T(m_netLoop, it->second);

	if (m_serverCfg.type == ServerType_MultiThread && cfg.inLoop) { // 独立线程服务
		m_inLoopServices.push_back(service);
	} else {														// loop线程服务
		m_notInLoopServices.push_back(service);
	}
}

// SocketLoop 加个inLoop标记
// OneThreadServer		单线程Server，用户loop、rpc接口实现和网络在同一个线程 loopOnce loop loopOnce(保证网络包发出去)
// TwoThreadServer		双线程Server，用户loop、rpc接口一个线程，网络一个线程
// MultiThreadServer	多线程Server，用户loop单独一个线程，网络单独一个线程，rpc接口多个线程

}
#endif
