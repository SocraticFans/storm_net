#include "handler_test.h"
#include <stdio.h>
#include <iostream>

using namespace std;

void SocketConnector::onConnect(Socket* s) {
	cout << "connected id: " << s->id << " "  << s->ip << ":" << s->port << endl;	
	s->send("Hello");
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
