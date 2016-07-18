#include "storm_server.h"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#include "util/util_log.h"
#include "util/util_file.h"
#include "proto/registry.h"


using namespace std;

namespace storm {

StormServer* g_stormServer = NULL;
// 配置常量
const static uint32_t kDefaultThreadNum = 1;
const static uint32_t kDefaultConnections = 65535;
const static uint32_t kDefaultQueueLen = 100000;
const static uint32_t kEmptyTimeOut = 10;
const static uint32_t kDefaultKeepAliveTime = 60;
const static uint32_t kDefaultTimeOut = 60;

static volatile bool g_running = false;
static volatile bool g_netRunning = false;

static void sighandler(int /*sig*/) {
	g_running = false;
}

void greenOutput(const string& content) {
	cout << "\033[1;32m" << content <<  "\033[0m" << endl;
}

void redOutput(const string& content) {
	cout << "\033[1;31m" << content <<  "\033[0m" << endl;
}

StormServer::StormServer()
:m_loopInterval(20000)
,m_netLoop(NULL) {
	m_netLoop = new SocketLoop();
	m_proxyMgr = new ServiceProxyManager(m_netLoop);
}

bool StormServer::isTerminate() {
	return !g_running;
}

void StormServer::setLoopInterval(uint64_t us) {
	m_loopInterval = us;
}

int StormServer::run(int argc, char** argv) {
	g_stormServer = this;
	m_netTimer.addTimer(500, std::bind(&StormServer::updateNet, this), true);
	try {
		parseConfig(argc, argv);
		// 非daemon模式，日志同时输出到屏幕,
		if (!m_option.isDaemon()) {
			LogManager::setLogStdOut(true);
		}

		cout << endl;
		// 杀老进程
		killOldProcess();

		// 启动日志
		startLog();

		if (m_option.isStop()) {
			exit(0);
		}

		savePidFile();
		cout << "starting server, pid: " << getpid() << endl;

		signal(SIGINT, sighandler);
		signal(SIGTERM, sighandler);
		signal(SIGPIPE, SIG_IGN);

		displayServer();

		// 网络线程
		startNetThread();

		// 用户初始化
		if (!init()) {
			redOutput("start server failed");
			return -1;
		}
		g_netRunning = true;

		// 异步日志模式
		if (!m_serverCfg.logSync) {
			LogManager::setLogSync(false);
			LogManager::startAsyncThread();
		}

		// 启动监听
		if (!startListener()) {
			redOutput("start service listen failed");
			return -1;
		}

		// 开始服务
		startService();

		// 到这一步就保证启动成功了
		greenOutput("start server success");
		LOG_DEBUG << "Server Started, pid: " << getpid();

		// 守护进程 
		if (m_option.isDaemon()) {
			daemon();
		}

		// 主循环入口
		mainEntry();

		// 结束
		terminateService();

		// 网络线程结束
		terminateNetThread();

		STORM_INFO << "Server Normal Exit";
		LogManager::finish();
		removePidFile();
	} catch (std::exception& e) {
		STORM_ERROR << "Server Error " << e.what();
		std::cout << "server error " << e.what() << std::endl;
		return -1;
	}
	return 0;
}

void StormServer::mainEntry() {
	g_running = true;
	while (g_running) {
		mainLoop();
		for (ServiceVector::iterator it = m_mainThreadServices.begin(); it != m_mainThreadServices.end(); ++it) {
			(*it)->update(0);
		}
		m_proxyMgr->updateInMainLoop();
		usleep(m_loopInterval);
	}
	// 主循环退出
	mainLoopDestory();
}

StormServer::~StormServer() {
	delete m_netLoop;
	for (ServiceVector::iterator it = m_mainThreadServices.begin(); it != m_mainThreadServices.end(); ++it) {
		delete *it;
	}
	for (ServiceVector::iterator it = m_extraThreadServices.begin(); it != m_extraThreadServices.end(); ++it) {
		delete *it;
	}
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		delete it->second;
	}
}

void StormServer::startLog() {
	string logPath = m_serverCfg.logPath + "/" + m_serverCfg.appName + "/";
	//TODO 分区路径
	logPath += m_serverCfg.serverName + "/";

	LogManager::initLog(logPath, m_serverCfg.appName + "." + m_serverCfg.serverName);
	LogManager::setLogLevel((LogLevel)m_serverCfg.logLevel);
	LogManager::setRollLogInfo("", m_serverCfg.logNum, m_serverCfg.logSize);
}

bool StormServer::startListener() {
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		if (it->second->startListen() < 0) {
			return false;
		}
	}

	return true;
}

void StormServer::startService() {
	for (ServiceVector::iterator it = m_mainThreadServices.begin(); it != m_mainThreadServices.end(); ++it) {
		(*it)->init();
	}

	for (ServiceVector::iterator it = m_extraThreadServices.begin(); it != m_extraThreadServices.end(); ++it) {
		(*it)->startThread();
	}
}

void StormServer::terminateService() {
	for (ServiceVector::iterator it = m_mainThreadServices.begin(); it != m_mainThreadServices.end(); ++it) {
		(*it)->destroy();
	}
	for (ServiceVector::iterator it = m_extraThreadServices.begin(); it != m_extraThreadServices.end(); ++it) {
		(*it)->terminateThread();
	}
}

void StormServer::startNetThread() {
	m_netThread = std::thread(&StormServer::netEntry, this);
}

void StormServer::terminateNetThread() {
	g_netRunning = false;
	m_netLoop->terminate();
	m_netThread.join();
}

void StormServer::netEntry() {
	while (!g_netRunning) {
		m_netLoop->runOnce(1000);
		m_netTimer.update(UtilTime::getNowMS());
		m_netLoop->runOnce(0);
		m_proxyMgr->updateInNetLoop();
	}
	while (g_netRunning) {
		m_netLoop->runOnce(1000);
		netLoop();	
		m_netTimer.update(UtilTime::getNowMS());
		m_netLoop->runOnce(0);
		m_proxyMgr->updateInNetLoop();
	}
	netLoopDestory();
	m_netLoop->runOnce(0);
}

void StormServer::setPacketParser(const std::string& name, SocketHandler::PacketParser parser) {
	ListenerMapType::iterator it = m_listeners.find(name);
	if (it != m_listeners.end()) {
		it->second->setPacketParser(parser);
	}
}

void StormServer::parseConfig(int argc, char** argv) {
	m_option.parse(argc, argv);

	string configFile = m_option.getConfigFile();
	if (configFile.empty()) {
		configFile = string(briefLogFileName(argv[0])) + ".storm.conf";
	}

	CConfig config;
	config.parseFile(configFile);

	const CConfig& svrCfg = config.getSubConfig("server");
	parseServerConfig(svrCfg);

	const CConfig& clientCfg = config.getSubConfig("client");
	parseClientConfig(clientCfg);
}

uint32_t StormServer::parserRunThread(const std::string& str) {
	string s = UtilString::trim(str);
	s = UtilString::tolower(s);

	uint32_t type = RunThread_Net;
	if (s == "main_thread") {
		type = RunThread_Main;
	} else if (s == "extra_thread") {
		type = RunThread_Extra;
	} else if (s == "net_thread") {
		type = RunThread_Net;
	}

	return type;
}

void StormServer::parseServerConfig(const CConfig& cfg) {
	m_serverCfg.appName = cfg.getCfg("app");
	m_serverCfg.serverName = cfg.getCfg("server");
	m_serverCfg.setName = cfg.getCfg("set", "");
	m_serverCfg.pidFileName = m_serverCfg.serverName + ".pid";


	m_serverCfg.logNum = cfg.getCfg<uint32_t>("log_num", 10);
	m_serverCfg.logSize = UtilString::parseHumanReadableSize(cfg.getCfg("log_size", "50M"));
	m_serverCfg.logLevel = LogManager::parseLevel(cfg.getCfg("log_level", "DEBUG"));
	m_serverCfg.logPath = cfg.getCfg("log_path", "./log/");
	m_serverCfg.logSync = cfg.getCfg("log_sync", 0);

	map<string, ServiceConfig>& allService = m_serverCfg.services;
	const map<string, CConfig>& servicesCfg = cfg.getAllSubConfig();
	for (map<string, CConfig>::const_iterator it = servicesCfg.begin(); it != servicesCfg.end(); ++it) {
		const CConfig& serviceCfg = it->second;
		string serviceName = serviceCfg.getCfg("service");
		if (allService.find(serviceName) != allService.end()) {
			throw std::runtime_error("duplicate service: " + serviceName);
		}
		ServiceConfig& service = allService[serviceName];
		service.name = serviceName;
		string runThread = serviceCfg.getCfg("run_thread", "net_thread");
		service.runThread = parserRunThread(runThread);
		service.host = serviceCfg.getCfg("host");
		service.port = serviceCfg.getCfg<uint32_t>("port");
		service.threadNum = serviceCfg.getCfg("thread", kDefaultThreadNum);
		service.maxConnections = serviceCfg.getCfg("maxConnections", kDefaultConnections);
		service.maxQueueLen = serviceCfg.getCfg("maxQueueLen", kDefaultQueueLen);
		service.keepAliveTime = serviceCfg.getCfg("keepAliveTime", kDefaultKeepAliveTime);
		service.emptyConnTimeOut = serviceCfg.getCfg("emptyConnTimeOut", kEmptyTimeOut);
		service.queueTimeout = serviceCfg.getCfg("queueTimeout", kDefaultTimeOut);
	}
}

void StormServer::parseClientConfig(const CConfig& cfg) {
//	m_clientCfg.registeryAddress = cfg.getCfg("registery");
	m_clientCfg.asyncThreadNum = cfg.getCfg<uint32_t>("asyncThread", 1);
	m_clientCfg.connectTimeOut = cfg.getCfg<uint32_t>("connectTimeOut", 3000);
	uint32_t update_time = cfg.getCfg<uint32_t>("update_time", 60);
	m_proxyMgr->setUpdateTime(update_time);
	if (cfg.hasConfigKey("registry")) {
		std::string registryStr = cfg.getCfg("registry");
		RegistryServiceProxy* registryProxy = m_proxyMgr->stringToProxy<RegistryServiceProxy>(registryStr);
		m_proxyMgr->setRegistryProxy(registryProxy);
		if (registryProxy == NULL) {
			STORM_ERROR << "invalid registry";
		}
	}
}

void StormServer::displayServer() {
	STORM_INFO << "\n" 
			   << "\t AppName " << m_serverCfg.appName << "\n"
		       << "\t ServerName " << m_serverCfg.serverName;

	map<string, ServiceConfig>& allService = m_serverCfg.services;
	for (map<string, ServiceConfig>::const_iterator it = allService.begin(); it != allService.end(); ++it) {
		const ServiceConfig& cfg = it->second;
		STORM_INFO << "\n"
					<< "Service: " << cfg.name << "\n"
					<< "\t RunThread: " << cfg.runThread << "\n"
					<< "\t Host: " << cfg.host << "\n"
					<< "\t Port: " << cfg.port << "\n"
					<< "\t ThreadNum: " << cfg.threadNum << "\n"
					<< "\t MaxConnections: " << cfg.maxConnections << "\n"
					<< "\t MaxQueueLen: " << cfg.maxQueueLen << "\n"
					<< "\t QueueTimeOut: " << cfg.queueTimeout;
	}
}

void StormServer::savePidFile() {
	int pid = getpid();
	UtilFile::saveToFile(m_serverCfg.pidFileName, UtilString::tostr(pid) + "\n");
}

void StormServer::removePidFile() {
	UtilFile::removeFile(m_serverCfg.pidFileName);
}

void StormServer::killOldProcess() {
	string pidStr = UtilFile::loadFromFile(m_serverCfg.pidFileName);
	if (pidStr.empty()) {
		return;
	}
	int pid = UtilString::strto<int>(pidStr);
	cout << "stoping old server, pid " << pid << endl;

	int ret = kill(pid, 15);
	if (ret == 0 || errno == 0) {
		cout << "old server exiting ..." << endl;
		while (UtilFile::isFileExists(m_serverCfg.pidFileName)) {
			usleep(500 * 1000);
		}
	} else if (errno == ESRCH) {
		redOutput("permission not enough");
		return;
	} else if (errno == EPERM) {
		redOutput("server not running");
		removePidFile();
		return;
	}

	cout << "old server exited" << endl;
}

void StormServer::daemon() {
	greenOutput("run in daemon");
	int fd = open("/dev/null", O_RDWR );
	if (fd != -1) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
	} else {
		close(0);
		close(1);
		close(2);
	}
}

void StormServer::status(std::string& out) {
	ostringstream oss;
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		StormListener* listener = it->second;
		oss << "Listener: " << listener->m_name << "\n"
			<< "\tip        : " << listener->m_ip << "\n"
			<< "\tport      : " << listener->m_port << "\n"
			<< "\tonline num: " << listener->getOnlinNum() << "\n";
	}
	out = oss.str();
}

void StormServer::updateNet() {
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		it->second->updateNet();
	}
}

}
