#include "storm_listener.h"

#include <stdio.h>
#include <iostream>

#include "util/util_protocol.h"

#include "storm_service.h"

using namespace std;

namespace storm {

StormListener::StormListener(SocketLoop* loop, bool inLoop)
	:SocketListener(loop),
	 m_inLoop(inLoop),
	 m_service(NULL) {

}

void StormListener::onAccept(Socket* s) {
	cout << m_name << ", accept id: " << s->id << " "  << s->ip << ":" << s->port << endl;	
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
	cout << m_name << ", close id: " << s->id << " "  << s->ip << ":" << s->port << " type " << closeType << endl;

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
	if (m_inLoop) {
		Connection conn(s->id, s->fd, s->ip, s->port);
		m_service->onRequest(conn, data, len);
	} else {
		Packet* packet = new Packet(s->id, s->fd, s->ip, s->port);
		packet->type = PacketType_Packet;
		packet->data.assign(data, len);
		m_queue.push_back(packet);
	}
}

void StormListener::updateLogic() {

}

void StormListener::updateNet() {

}

}
