#include "storm_server.h"
#include <unistd.h>

namespace storm {

StormServer::StormServer()
: m_netLoop(NULL) {
	m_netLoop = new SocketLoop();
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

int StormServer::run(int argc, char** argv) {
	try {
		parseConfig(argc, argv);
		// 初始化
		if (!init()) {
			// TODO 日志
			return -1;
		}

		uint32_t type = m_serverCfg.type;

		if (type == ServerType_SingleThread) {
			// 启动监听，网络loop处理逻辑
			startService(true);
		} else if (type == ServerType_MultiThread) {
			// 启动监听，网络loop不处理逻辑
			startService(false);
			// 网络线程
			startNetThread();
		}

		if (type == ServerType_SingleThread) {
			while (1) {
				m_netLoop->runOnce(2);
				loop();
			}
		} else if (type == ServerType_MultiThread) {
			// 开启逻辑需要独立线程的Service线程
			startServiceThread();
			// loop线程
			while (1) {
				loop();
				for (ServiceVector::iterator it = m_inLoopServices.begin(); it != m_inLoopServices.end(); ++it) {
					(*it)->update(0);
				}
				usleep(2000);
			}
		}

	} catch (std::exception& e) {
	//	STORM_ERROR << "server error " << e.what();
		return -1;
	}
	return 0;
}

void StormServer::setServerType(ServerType type) {
	m_serverCfg.type = type;
}

void StormServer::terminate() {

}

bool StormServer::startService(bool inLoop) {
	m_netLoop->setCmdInLoop(inLoop);
	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		it->second->setInLoop(inLoop);
	}

	for (ListenerMapType::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
		if (it->second->startListen() < 0) {
			return false;
		}
	}

	return true;
}

void StormServer::startServiceThread() {
	for (ServiceVector::iterator it = m_notInLoopServices.begin(); it != m_notInLoopServices.end(); ++it) {
		(*it)->startThread();
	}
}

void StormServer::startNetThread() {
	m_netThread = std::thread(&StormServer::netLoop, this);
}

void StormServer::netLoop() {
	// TODO 结束
	while (1) {
		m_netLoop->runOnce(1000);
		// TODO listener 的updateNet方法等等
	}
}

void StormServer::setPacketParser(const std::string& name, SocketListener::PacketParser parser) {
	ListenerMapType::iterator it = m_listeners.find(name);
	if (it != m_listeners.end()) {
		it->second->setPacketParser(parser);
	}
}

void StormServer::parseConfig(int argc, char** argv) {

}

void StormServer::parseServerConfig(const CConfig& cfg) {

}

void StormServer::parseClientConfig(const CConfig& cfg) {

}

}
