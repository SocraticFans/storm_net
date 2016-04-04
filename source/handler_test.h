#ifndef HANDLER_TEST_H_
#define HANDLER_TEST_H_

#include "net/socket_define.h"
#include "net/socket_loop.h"

using namespace storm;

class SocketConnector : public SocketHandler {
public:
	SocketConnector(SocketLoop* loop)
		:m_loop(loop) {

	}

	virtual void onConnect(Socket* s);
	virtual bool onData(Socket* s);
	virtual void onClose(Socket* s, uint32_t closeType);

private:
	SocketLoop* m_loop;
};

#endif
