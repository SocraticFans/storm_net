#include "storm_server.h"

#include <iostream>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <fcntl.h>

#include "util/util_log.h"
#include "util/util_file.h"


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

static bool g_running = false;

static void sighandler(int /*sig*/) {
	g_running = false;
}

void greenOutput(const string& content) {
	cout << "\033[1;32m" << content <<  "\033[0m" << endl;
}

void redOutput(const string& content) {
	cout << "\033[1;31m" << content <<  "\033[0m" << endl;
}

bool StormServer::isTerminate() {
	return !g_running;
}

int StormServer::run(int argc, char** argv) {
	g_stormServer = this;
	m_netTimer.addTimer(500, std::bind(&StormServer::updateNet, this), true);
	try {
		parseConfig(argc, argv);
		if (!m_option.isDaemon()) {
			// 非daemon模式，日志同时输出到屏幕,
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

		// 用户初始化
		if (!init()) {
			redOutput("start server failed");
			return -1;
		}
		// 异步日志模式
		if (!m_serverCfg.logSync) {
			LogManager::setLogSync(false);
			LogManager::startAsyncThread();
		}
		// 网络线程
		startNetThread();
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

		// 主循环
		mainLoop();
		// 用户退出
		destroy();
		// 结束
		terminate();

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

void StormServer::mainLoop() {
	g_running = true;
	uint32_t type = m_serverCfg.type;
	if (type == ServerType_SingleThread) {
		while (g_running) {
			loop();
			m_netLoop->runOnce(2);
			m_netTimer.update(UtilTime::getNowMS());
		}
	} else if (type == ServerType_MultiThread) {
		while (g_running) {
			loop();
			for (ServiceVector::iterator it = m_inLoopServices.begin(); it != m_inLoopServices.end(); ++it) {
				(*it)->update(0);
			}
			usleep(2000);
		}
	}
}

StormServer::StormServer()
: m_netLoop(NULL) {
	m_netLoop = new SocketLoop();
	m_proxyMgr = new ServiceProxyManager(m_netLoop);
}

StormServer::~StormServer() {
	delete m_netLoop;
	for (ServiceVector::iterator it = m_inLoopServices.begin(); it != m_inLoopServices.end(); ++it) {
		delete *it;
	}
	for (ServiceVector::iterator it = m_notInLoopServices.begin(); it != m_notInLoopServices.end(); ++it) {
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

void StormServer::setServerType(ServerType type) {
	m_serverCfg.type = type;
}

void StormServer::terminate() {
	terminateService();
	terminateNetThread();
}

bool StormServer::startListener() {
	bool netInloop = true;
	if (m_serverCfg.type == ServerType_SingleThread) {
		// 网络loop处理逻辑
		netInloop = true;
	} else if (m_serverCfg.type == ServerType_MultiThread) {
		// 网络loop不处理逻辑
		netInloop  = false;
	}

	m_netLoop->setCmdInLoop(netInloop);
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		it->second->setInLoop(netInloop);
	}

	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		if (it->second->startListen() < 0) {
			return false;
		}
	}

	return true;
}

void StormServer::startService() {
	for (ServiceVector::iterator it = m_inLoopServices.begin(); it != m_inLoopServices.end(); ++it) {
		(*it)->init();
	}

	for (ServiceVector::iterator it = m_notInLoopServices.begin(); it != m_notInLoopServices.end(); ++it) {
		(*it)->startThread();
	}
}

void StormServer::terminateService() {
	for (ServiceVector::iterator it = m_inLoopServices.begin(); it != m_inLoopServices.end(); ++it) {
		(*it)->destroy();
	}
	for (ServiceVector::iterator it = m_notInLoopServices.begin(); it != m_notInLoopServices.end(); ++it) {
		(*it)->terminateThread();
	}
}

void StormServer::startNetThread() {
	if (m_serverCfg.type != ServerType_MultiThread) {
		return;
	}
	m_netThread = std::thread(&StormServer::netLoop, this);
}

void StormServer::terminateNetThread() {
	m_netLoop->terminate();
	if (m_serverCfg.type != ServerType_MultiThread) {
		return;
	}
	m_netThread.join();
}

void StormServer::netLoop() {
	while (g_running) {
		m_netLoop->runOnce(2);
		m_netTimer.update(UtilTime::getNowMS());
	}
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
		configFile = string(briefLogFileName(argv[0])) + ".conf";
	}

	CConfig config;
	config.parseFile(configFile);

	const CConfig& svrCfg = config.getSubConfig("server");
	parseServerConfig(svrCfg);

	const CConfig& clientCfg = config.getSubConfig("client");
	parseClientConfig(clientCfg);
}

ServerType StormServer::parserServerType(const std::string& str) {
	string s = UtilString::trim(str);
	s = UtilString::toupper(s);

	ServerType type = ServerType_MultiThread;
	if (s == "SINGLE_THREAD") {
		type = ServerType_SingleThread;
	} else if (s == "MULTI_THREAD") {
		type = ServerType_MultiThread;
	}
	return type;
}

void StormServer::parseServerConfig(const CConfig& cfg) {
	m_serverCfg.appName = cfg.getCfg("app");
	m_serverCfg.serverName = cfg.getCfg("server");
	m_serverCfg.pidFileName = m_serverCfg.serverName + ".pid";
	
	string serverType = cfg.getCfg("type", "Multi_Thread");
	m_serverCfg.type = parserServerType(serverType);

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
		// TODO getCfg 模板特化一个bool参数
		service.inLoop = serviceCfg.getCfg("inLoop", 1);
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
}

void StormServer::displayServer() {
	STORM_INFO << "\n" 
			   << "\t AppName " << m_serverCfg.appName << "\n"
		       << "\t ServerName " << m_serverCfg.serverName << "\n"
			   << "\t Type " << m_serverCfg.type;

	map<string, ServiceConfig>& allService = m_serverCfg.services;
	for (map<string, ServiceConfig>::const_iterator it = allService.begin(); it != allService.end(); ++it) {
		const ServiceConfig& cfg = it->second;
		STORM_INFO << "\n"
					<< "Service: " << cfg.name << "\n"
					<< "\t InLoop: " << cfg.inLoop << "\n"
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
