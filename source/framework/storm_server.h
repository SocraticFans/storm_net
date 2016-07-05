#ifndef STORM_SERVER_H_
#define STORM_SERVER_H_

#include <map>
#include <thread>

#include "net/socket_loop.h"

#include "util/util_config.h"
#include "util/util_option.h"
#include "util/util_string.h"
#include "util/util_timer.h"

#include "server_config.h"
#include "storm_service.h"
#include "storm_listener.h"
#include "storm_proxy_manager.h"

namespace storm {

class StormServer {
public:
	StormServer();
	virtual ~StormServer();

	int run(int argc, char** argv);

	virtual bool init() = 0;
	virtual void destroy() = 0;
	virtual void loop() = 0;

	void setServerType(ServerType type);

	void status(std::string& out);

	bool isTerminate();

protected:
	// 主循环
	void mainLoop();

	// 启动日志
	void startLog();

	// 开始服务监听
	bool startListener();

	// 开始服务
	void startService();

	// 关闭服务
	void terminateService();

	// 开启网络线程
	void startNetThread();

	// 结束网络线程
	void terminateNetThread();

	// 网络loop
	void netLoop();

	// 结束
	void terminate();

	// 网络线程定时
	void updateNet();


	void setPacketParser(const std::string& name, SocketHandler::PacketParser parser);

	void parseConfig(int argc, char** argv);
	void parseServerConfig(const CConfig& cfg);
	void parseClientConfig(const CConfig& cfg);
	ServerType parserServerType(const std::string& str);
	void displayServer();

	void savePidFile();
	void removePidFile();
	void killOldProcess();
	void daemon();

	template<typename T>
	void addService(const std::string& serviceName, SocketHandler::PacketParser parser = NULL) {
		std::map<string, ServiceConfig>& allService = m_serverCfg.services;
		std::map<string, ServiceConfig>::iterator it = allService.find(serviceName);
		if (it == allService.end()) {
			throw std::runtime_error("cannot find service config, service: " + serviceName);
		}
		if (!addService<T>(it->second, parser)) {
			throw std::runtime_error("add service error, service: " + serviceName);
		}
	}

	template <class T>
	bool addService(const ServiceConfig& cfg, SocketHandler::PacketParser parser = NULL);

protected:
	typedef std::map<std::string, StormListener*> ListenerMapType;
	typedef std::vector<StormService*> ServiceVector;

	COption 		m_option;				// 命令行选项
	ServerConfig 	m_serverCfg;			// 服务端配置
	ClientConfig 	m_clientCfg;			// 客户端配置

	SocketLoop*		m_netLoop;				// 网络loop实例
	ServiceProxyManager* m_proxyMgr;			// proxy管理器

	ListenerMapType m_listeners;			// 所有的监听器
	ServiceVector	m_inLoopServices;		// 在loop中处理逻辑的Service
	ServiceVector	m_notInLoopServices;	// 不在loop中处理逻辑的Service
	std::thread 	m_netThread;			// 网络线程
	Timer			m_netTimer;				// 网络线程里的定时器
};

template <class T>
bool StormServer::addService(const ServiceConfig& cfg, SocketHandler::PacketParser parser) {
	//const std::string& key = cfg.name + cfg.ip + UtilString::tostr(cfg.port);
	const std::string& key = cfg.name;

	// 创建监听器
	ListenerMapType::iterator it = m_listeners.find(key);
	if (it == m_listeners.end()) {
		StormListener* l = new StormListener(m_netLoop);
		l->setName(cfg.name);
		l->setIp(cfg.host);
		l->setPort(cfg.port);
		l->m_config = cfg;
		it = m_listeners.insert(std::make_pair(key, l)).first;
	}

	int32_t num = 1;
	if (m_serverCfg.type == ServerType_MultiThread && !cfg.inLoop) {
		num = cfg.threadNum;
	}
	for (int32_t i = 0; i < num; ++i) {
		// 创建服务对象
		T* service = new T(m_netLoop, it->second);

		if (m_serverCfg.type == ServerType_SingleThread) {
			m_inLoopServices.push_back(service);
		} else if (m_serverCfg.type == ServerType_MultiThread) {
			if (cfg.inLoop) {
				m_inLoopServices.push_back(service);
			} else {
				m_notInLoopServices.push_back(service);
			}
		}
	}

	// 设置分包解析器
	if (parser) {
		it->second->setPacketParser(parser);
	}

	return true;
}
extern StormServer* g_stormServer;
}
#endif
