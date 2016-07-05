#include "storm_listener.h"

#include <stdio.h>
#include <iostream>

#include "util/util_protocol.h"
#include "util/util_log.h"
#include "util/util_time.h"

#include "storm_service.h"

using namespace std;

namespace storm {

StormListener::StormListener(SocketLoop* loop, bool inLoop)
	:SocketHandler(loop),
	 m_inLoop(inLoop),
	 m_service(NULL) {

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
}

void StormListener::doTimeClose(uint32_t id) {
	m_loop->close(id, CloseType_Timeout);
}

void StormListener::doIdleClose(uint32_t id) {
	m_loop->close(id, CloseType_IdleTimeout);
}

}
