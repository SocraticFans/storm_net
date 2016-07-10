#include "ServiceManager.h"
#include "util/util_log.h"
#include "util/util_time.h"

#include "RegistryConfig.h"

using namespace storm;

namespace Registry {
bool ServiceManager::init() {
	m_mysql.setConfig(g_config.dbConfig);
	m_tblName = "t_storm";

	loadFromDB();

	return true;
}

void ServiceManager::loadFromDB() {
	LOG_INFO;
	m_reloadSec = UtilTime::getNow();
	ServiceMap services;
	try {
		MySqlResult::ptr res = m_mysql.select(m_tblName, "*");
		if (res->size() == 0) {
			return;
		}
		for (uint32_t i = 0; i < res->size(); ++i) {
			EndPoint ep;
			ep.appName = res->get(i, "app_name");
			ep.serverName = res->get(i, "server_name");
			ep.serviceName = res->get(i, "service_name");
			ep.setName = res->get(i, "set_name");
			ep.ip = res->get(i, "ip");
			ep.port = res->get<uint32_t>(i, "port");
			std::string key = ep.appName + ep.serverName + ep.serviceName + ep.setName;
			services[key].push_back(ep);
		}

	} catch (std::exception& e) {
		LOG_ERROR << e.what();
		return;
	}
	ScopeMutex<Mutex> lock(m_mutex);
	m_services.swap(services);
}

void ServiceManager::addNewService(const EndPoint& ep) {
	ScopeMutex<Mutex> lock(m_newMutex);
	m_newServices.push_back(ep);
}

void ServiceManager::flushNewServiceToDB() {
	ScopeMutex<Mutex> lock(m_newMutex);
	if (m_newServices.empty()) {
		return;
	}
	std::vector<EndPoint> eps;
	eps.swap(m_newServices);
	lock.unlock();

	try {
		for (auto it = eps.begin(); it != eps.end(); ++it) {
			const EndPoint& ep = *it;
			SqlJoin sql;
			sql << SqlPair("app_name", ep.appName)
				<< SqlPair("server_name", ep.serverName)
				<< SqlPair("service_name", ep.serviceName)
				<< SqlPair("set_name", ep.setName)
				<< SqlPair("ip", ep.ip)
				<< SqlPair("port", ep.port);
			m_mysql.insert(m_tblName, sql);
		}
	} catch (std::exception& e) {
		LOG_ERROR << e.what();
	}
}

void ServiceManager::update() {
	uint32_t now = UtilTime::getNow();
	flushNewServiceToDB();
	if (now > (m_reloadSec + 60)) {
		loadFromDB();
	}
}

void ServiceManager::getService(vector<EndPoint>& services, const string& appName,
								const string& serverName, const string& serviceName, const string& setName) {
	std::string key = appName + serverName + serviceName + setName;

	ScopeMutex<Mutex> lock(m_mutex);
	ServiceMap::iterator it = m_services.find(key);
	if (it == m_services.end()) {
		return;
	}
	services = it->second;
}
}
