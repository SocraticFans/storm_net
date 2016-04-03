#include "handler_test.h"
#include <stdio.h>
#include <iostream>

using namespace std;
bool SocketListener::onAccept(Socket* s) {
	cout << "accept id: " << s->id << " "  << s->ip << ":" << s->port << endl;	
	return true;
}

bool SocketListener::onData(Socket* s) {
	IoBuffer* buffer = s->readBuffer;
	string data(buffer->getHead(), buffer->getSize());
	buffer->readN(data.size());

	cout << "recv: " << data << endl;

	if (data == "exit\r\n") {
		cout << "退出" << endl;
		m_loop->terminate();
	}
	if (data == "close\r\n") {
		m_loop->close(s->id);
	}

	m_loop->send(s->id, data);
	return true;
}

void SocketListener::onClose(Socket* s, uint32_t closeType) {
	cout << "close id: " << s->id << " "  << s->ip << ":" << s->port << " type " << closeType << endl;
}

void SocketConnector::onConnect(Socket* s) {
	cout << "connected id: " << s->id << " "  << s->ip << ":" << s->port << endl;	
	m_loop->send(s->id, "Hello");
}

bool SocketConnector::onData(Socket* s) {
	IoBuffer* buffer = s->readBuffer;
	string data(buffer->getHead(), buffer->getSize());
	buffer->readN(data.size());

	cout << "connector recv: " << data << endl;
	return true;
}

void SocketConnector::onClose(Socket* s, uint32_t closeType) {
	cout << "connector close id: " << s->id << " " << s->ip << ":" << s->port << " type " << closeType << endl;
}
