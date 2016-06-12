#ifndef SERVER_CONFIG_H_
#define SERVER_CONFIG_H_

#include <stdint.h>
#include <string>
#include <map>

namespace storm {

enum ServerType {
	ServerType_SingleThread = 1,	// 单线程Server 网络线程、网络包处理、用户loop都在一个线程里
	ServerType_MultiThread = 2,		// 多线程Server 网络loop一个线程，用户loop一个线程，网络逻辑可以放在用户loop线程，也可以独立线程(可以配置多个)
};

struct ServiceConfig {
	bool inLoop; 					// 在多线程Server模式，配置网络处理是否在loop线程，单线程Server模式无意义
	string name;
	string ip;
	uint32_t port;
	uint32_t threadNum;				// 多线程模式，网络逻辑不在loop中时才有意义
	uint32_t maxConnections;
	uint32_t maxQueueLen;
	uint32_t keepAliveTime;
	uint32_t emptyConnTimeOut;
	uint32_t queueTimeout;
};

struct ServerConfig {
	uint32_t type;
	std::string appName;
	std::string serverName;

	std::string basePath;
	std::string dataPath;
	std::string localIp;
	std::string logPath;

	std::string local;
	std::string node;
	std::string log;
	std::string config;
	std::string notify;
	std::string configFile;
	int         reportFlow;
	int         isCheckSet;

	uint32_t logNum;
	uint64_t logSize;
	uint32_t logLevel;

	std::map<string, ServiceConfig> services;
};

struct ClientConfig {
	string registeryAddress;
	uint32_t connectTimeOut;
	uint32_t asyncThreadNum;
};

}

#endif
