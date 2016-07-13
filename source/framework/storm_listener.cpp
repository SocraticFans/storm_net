#include "storm_listener.h"

#include <stdio.h>
#include <iostream>

#include "util/util_protocol.h"
#include "util/util_log.h"
#include "util/util_time.h"

#include "storm_service.h"
#include "storm_proxy_manager.h"
#include "storm_server.h"

using namespace std;

namespace storm {

StormListener::StormListener(SocketLoop* loop, bool inLoop)
	:SocketHandler(loop),
	 m_inLoop(inLoop),
	 m_service(NULL),
	 m_lastHeartBeatSec(0) {

}

void StormListener::onAccept(Socket* s) {
	STORM_INFO <<  m_name << ", accept id: " << s->id << " "  << s->ip << ":" << s->port;
	if (m_timelist.size() >= m_config.maxConnections) {
		STORM_ERROR << "max Connection";
		m_loop->close(s->id, CloseType_Self);
		return;
	}

	// 加入空连接检查队列
	uint32_t now = UtilTime::getNow();
	m_conList.add(s->id, now);

	if (m_inLoop) {
		Connection conn(s->id, s->fd, s->ip, s->port);
		m_service->onAccept(conn);
	} else {
		Packet* packet = new Packet(s->id, s->fd, s->ip, s->port);
		packet->type = PacketType_Accept;
		m_queue.push_back(packet);
	}
}

void StormListener::onClose(Socket* s, uint32_t closeType) {
	STORM_INFO << m_name << ", close id: " << s->id << " "  << s->ip << ":" << s->port << " type " << etos((SocketCloseType)closeType);

	m_conList.del(s->id);
	m_timelist.del(s->id);

	if (m_inLoop) {
		Connection conn(s->id, s->fd, s->ip, s->port);
		m_service->onClose(conn, closeType);
	} else {
		Packet* packet = new Packet(s->id, s->fd, s->ip, s->port);
		packet->type = PacketType_Close;
		packet->closeType = closeType;
		m_queue.push_back(packet);
	}
}

void StormListener::onPacket(Socket* s, const char* data, uint32_t len) {
	uint32_t now = UtilTime::getNow();
	m_conList.del(s->id);
	m_timelist.add(s->id, now);
	if (m_inLoop) {
		Connection conn(s->id, s->fd, s->ip, s->port);
		m_service->onRequest(conn, data, len);
	} else {
		Packet* packet = new Packet(s->id, s->fd, s->ip, s->port);
		packet->type = PacketType_Packet;
		packet->data.assign(data, len);

		// 待处理的包太多，直接扔掉
		if (m_queue.size() > m_config.maxQueueLen) {
			STORM_ERROR << "service: " << m_name << ", queue size over max";
			delete packet;
			return;
		}
		m_queue.push_back(packet);
	}
}

int32_t StormListener::startListen() {
	// 设置超时检测
	m_conList.setTimeout(m_config.emptyConnTimeOut);
	m_conList.setFunction(std::bind(&StormListener::doIdleClose, this, std::placeholders::_1));
	m_timelist.setTimeout(m_config.keepAliveTime);
	m_timelist.setFunction(std::bind(&StormListener::doTimeClose, this, std::placeholders::_1));

	return m_loop->listen(this, m_ip, m_port);
}

void StormListener::updateLogic() {

}

void StormListener::updateNet() {
	uint32_t now = UtilTime::getNow();
	m_conList.timeout(now);
	m_timelist.timeout(now);

	RegistryServiceProxy* proxy = g_stormServer->getProxyManager()->getRegistryProxy();
	if (now > m_lastHeartBeatSec + 30 && proxy) {
		STORM_INFO;
		const ServerConfig& serverConfig = g_stormServer->getServerConfig();
		ServiceHeartBeatReq req;
		ServiceInfo* info = req.mutable_info();
		info->set_app_name(serverConfig.appName);
		info->set_server_name(serverConfig.serverName);
		info->set_service_name(m_config.name);
		info->set_set_name(serverConfig.setName);
		info->set_ip(m_config.host);
		info->set_port(m_config.port);
		proxy->async_HeartBeat(NULL, req);
		m_lastHeartBeatSec = now;
	}
}

void StormListener::doTimeClose(uint32_t id) {
	m_loop->close(id, CloseType_Timeout);
}

void StormListener::doIdleClose(uint32_t id) {
	m_loop->close(id, CloseType_IdleTimeout);
}

}
