#include "service.h"
#include "listener.h"

#include <stdio.h>
#include <iostream>

namespace storm {

Service::Service(SocketLoop* loop, Listener* listener)
	:m_loop(loop),
	 m_listener(listener) {
	m_listener->setService(this);
}

void Service::doRequest(const Connection& conn, const char* buffer, uint32_t len) {
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

}
