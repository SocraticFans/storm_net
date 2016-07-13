#ifndef SERVER_CONFIG_H_
#define SERVER_CONFIG_H_

#include <stdint.h>
#include <string>
#include <map>

namespace storm {

enum RunThread {
	RunThread_Main = 1,				// 运行在主线程
	RunThread_Net = 2,				// 运行在网络线程
	RunThread_Extra = 3,			// 运行在额外线程
};

struct ServiceConfig {
	uint32_t runThread;				// 运行线程
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
	std::string appName;			// 应用名称
	std::string serverName;			// 服务名称
	std::string setName;			// set名字
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
