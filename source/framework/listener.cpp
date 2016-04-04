#include "listener.h"

#include <stdio.h>
#include <iostream>

#include "util/util_protocol.h"
#include "service.h"

using namespace std;

namespace storm {

Listener::Listener(SocketLoop* loop, bool inLoop)
	:SocketListener(loop),
	 m_inLoop(inLoop) {

}

void Listener::onAccept(Socket* s) {
	cout << "accept id: " << s->id << " "  << s->ip << ":" << s->port << endl;	
}

void Listener::onClose(Socket* s, uint32_t closeType) {
	cout << "close id: " << s->id << " "  << s->ip << ":" << s->port << " type " << closeType << endl;
}

void Listener::onPacket(Socket* s, const char* data, uint32_t len) {
	if (m_inLoop && m_service) {
		Connection conn(s->id, s->fd, s->ip, s->port);
		m_service->doRequest(conn, data, len);
	} else {

	}
}

}
