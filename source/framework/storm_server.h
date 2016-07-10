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

#include "proto/registry.h"

namespace storm {

class StormServer {
public:
	StormServer();
	virtual ~StormServer();

	int run(int argc, char** argv);

	virtual bool init() = 0;

	virtual void mainLoop() {}				// 主循环
	virtual void mainLoopDestory() {}		// 主循环退出

	virtual void netLoop() {};				// 网络循环
	virtual void netLoopDestory() {};		// 网络循环退出

	void status(std::string& out);

	bool isTerminate();

	void setLoopInterval(uint64_t us);

protected:
	// 主循环入口
	void mainEntry();

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

	// 网络Entry
	void netEntry();

	// 网络线程定时
	void updateNet();


	void setPacketParser(const std::string& name, SocketHandler::PacketParser parser);

	void parseConfig(int argc, char** argv);
	void parseServerConfig(const CConfig& cfg);
	void parseClientConfig(const CConfig& cfg);
	uint32_t parserRunThread(const std::string& str);
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

	uint64_t m_loopInterval;				// loop间隔
	COption 		m_option;				// 命令行选项
	ServerConfig 	m_serverCfg;			// 服务端配置
	ClientConfig 	m_clientCfg;			// 客户端配置

	SocketLoop*		m_netLoop;				// 网络loop实例
	ServiceProxyManager* m_proxyMgr;			// proxy管理器

	ListenerMapType m_listeners;			// 所有的监听器
	ServiceVector	m_mainThreadServices;	// 逻辑线程中运行的Service
	ServiceVector	m_extraThreadServices;	// 额外的线程运行的Service
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
	if (cfg.runThread == RunThread_Extra) {
		num = cfg.threadNum;
	}
	for (int32_t i = 0; i < num; ++i) {
		// 创建服务对象
		T* service = new T(m_netLoop, it->second);

		if (cfg.runThread == RunThread_Main) {
			m_mainThreadServices.push_back(service);
		} else if (cfg.runThread == RunThread_Extra) {
			m_extraThreadServices.push_back(service);
		}
	}

	if (cfg.runThread == RunThread_Net) {
		m_netLoop->setCmdInLoop(true);
		it->second->setInLoop(true);
	} else {
		m_netLoop->setCmdInLoop(false);
		it->second->setInLoop(false);
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
