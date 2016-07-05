#ifndef TEST_ECHO_SERVICE_H
#define TEST_ECHO_SERVICE_H

#include "framework/storm_service.h"

#include "echo.pb.h"

using namespace storm;

class EchoService : public storm::StormService {
public:
	EchoService(SocketLoop* loop, StormListener* listener);
	virtual ~EchoService() {}

	virtual int onRpcRequest(const Connection& conn, const RpcRequest& req, RpcResponse& resp);

	virtual void Echo(const Connection& conn, const EchoReq& req, EchoAck& ack);
};

#endif

