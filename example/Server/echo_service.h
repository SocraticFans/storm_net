#ifndef TEST_ECHO_SERVICE_H
#define TEST_ECHO_SERVICE_H

#include "echo.h"

using namespace storm;

class EchoServiceImp : public EchoService {
public:
	EchoServiceImp(SocketLoop* loop, StormListener* listener)
		:EchoService(loop, listener) {}
	virtual ~EchoServiceImp() {}

	virtual int32_t Echo(const Connection& conn, const EchoReq& req, EchoAck& ack);
};

#endif

