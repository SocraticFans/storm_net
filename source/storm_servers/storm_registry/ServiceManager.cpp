#include "ServiceManager.h"
#include "util/util_log.h"
#include "util/util_time.h"
#include "util/util_string.h"

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
	std::set<std::string> allServiceStr;
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
			std::string ipPort =  ep.ip + "-" + UtilString::tostr(ep.port);
			allServiceStr.insert(ipPort);
		}

	} catch (std::exception& e) {
		LOG_ERROR << e.what();
		return;
	}

	{
		ScopeMutex<Mutex> lock(m_mutex);
		m_services.swap(services);
	}
	{
		ScopeMutex<Mutex> lock(m_mutexAllService);
		m_allService.swap(allServiceStr);
	}

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

bool ServiceManager::isNewService(const std::string& ip, uint32_t port) {
	std::string ipPort =  ip + "-" + UtilString::tostr(port);
	ScopeMutex<Mutex> lock(m_mutexAllService);
	return m_allService.find(ipPort) == m_allService.end();
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

void ServiceManager::heartBeat(const ServiceInfo& req) {
	bool isNew = isNewService(req.ip(), req.port());
	std::string key = req.app_name() + req.server_name() + req.service_name() + req.set_name();
	if (isNew) {
		EndPoint ep;
		ep.appName = req.app_name();
		ep.serverName = req.server_name();
		ep.serviceName = req.service_name();
		ep.setName = req.set_name();
		ep.ip = req.ip();
		ep.port = req.port();
		ep.lastHeartBeatTime = UtilTime::getNow();
		ep.active = true;

		addNewService(ep);

		ScopeMutex<Mutex> lock(m_mutex);
		m_services[key].push_back(ep);
	} else {
		ScopeMutex<Mutex> lock(m_mutex);
		std::vector<EndPoint> vec = m_services[key];
		for (auto it = vec.begin(); it != vec.end(); ++it) {
			EndPoint& ep = *it;
			if (ep.ip == req.ip() && ep.port == req.port()) {
				ep.lastHeartBeatTime = UtilTime::getNow();
				ep.active = true;
			}
		}
	}

}

}
