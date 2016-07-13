#ifndef _STORM_REGISTRY_SERVICE_MANAGER_H_
#define _STORM_REGISTRY_SERVICE_MANAGER_H_

#include <map>
#include <string>
#include <vector>
#include <set>

#include "util/util_thread.h"
#include "util/util_singleton.h"
#include "util/util_mysql.h"

#include "proto/registry.pb.h"

using namespace std;
using namespace storm;

namespace Registry {

struct EndPoint {
	EndPoint()
		:lastHeartBeatTime(0), active(true) {}
	string appName;
	string serverName;
	string serviceName;
	string setName;
	string ip;
	uint32_t port;
	uint32_t lastHeartBeatTime;
	bool active;
};

class ServiceManager : public Singleton<ServiceManager> {
public:
	bool init();

	void update();

	void getService(vector<EndPoint>& services, const string& appName, const string& serverName, const string& serviceName, const string& setName);

	void heartBeat(const ServiceInfo& req);

private:

	void loadFromDB();
	void addNewService(const EndPoint& ep);
	void flushNewServiceToDB();

	bool isNewService(const std::string& ip, uint32_t port);

private:
	// key = appName + serverName + serviceName + setName
	typedef std::map<std::string, std::vector<EndPoint> > ServiceMap;

	Mutex m_mutex;
	ServiceMap m_services;

	Mutex m_newMutex;
	std::vector<EndPoint> m_newServices;

	Mutex m_mutexAllService;
	std::set<std::string> m_allService;	// ip + port 组成的key

	uint32_t m_reloadSec;
	MySqlConn m_mysql;
	string m_tblName;
};

}
#endif
