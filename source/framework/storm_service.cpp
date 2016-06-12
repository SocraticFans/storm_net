#include "storm_service.h"

#include <stdio.h>
#include <iostream>

#include "storm_listener.h"

namespace storm {

StormService::StormService(SocketLoop* loop, StormListener* listener)
	:m_loop(loop),
	 m_listener(listener) {
	m_listener->setService(this);
}

void StormService::onClose(const Connection& conn, uint32_t closeType) {

}

void StormService::onRequest(const Connection& conn, const char* buffer, uint32_t len) {
	string data(buffer, len);
	cout << "recv: " << data << endl;

	if (data == "exit") {
		cout << "退出" << endl;
		m_loop->terminate();
	}
	if (data == "close") {
		m_loop->close(conn.id);
	}

	m_loop->send(conn.id, data + "\n");
}

void StormService::startThread() {
	m_thread = std::thread(&StormService::loop, this);
}

void StormService::loop() {
	// TODO 结束
	while (1) {
		update(-1);
	}
}

void StormService::update(int64_t ms) {
	StormListener::Packet* packet;
	while (m_listener->m_queue.pop_front(packet, ms)) {
		switch (packet->type) {
			case StormListener::PacketType_Accept:
				onAccept(packet->conn);
				break;
			case StormListener::PacketType_Close:
				onClose(packet->conn, packet->closeType);
				break;
			case StormListener::PacketType_Packet:
				onRequest(packet->conn, packet->data.c_str(), packet->data.size());
				break;
		}
		delete packet;
	}
}

}
