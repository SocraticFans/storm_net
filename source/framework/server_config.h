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
	string name;					// 服务名字
	string host;					// 监听ip
	uint32_t port;					// 监听port
	uint32_t threadNum;				// 多线程模式，网络逻辑不在loop中时才有意义
	uint32_t maxConnections;		// 最大连接数
	uint32_t maxQueueLen;			// 最大包队列长度
	uint32_t keepAliveTime;			// keep alive时间
	uint32_t emptyConnTimeOut;		// 空连接时间
	uint32_t queueTimeout;			// 队列超时
};

struct ServerConfig {
	uint32_t type;					// server类型，查看ServerType枚举
	std::string appName;			// 应用名称
	std::string serverName;			// 服务名称
	std::string pidFileName;		// pid文件名称

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

	uint32_t logNum;				// 滚动日志最多文件个数
	uint64_t logSize;				// 滚动日志最大空间
	uint32_t logLevel;				// 日志等级
	bool	logSync;				// 日志是否同步
	bool	logStdOut;				// 日志是否输出到标准输出

	std::map<string, ServiceConfig> services;	// 所有的服务配置
};

struct ClientConfig {
	string registeryAddress;
	uint32_t connectTimeOut;
	uint32_t asyncThreadNum;
};

}

#endif
