#include "echo_service.h"
#include "util/util_time.h"

EchoService::EchoService(SocketLoop* loop, StormListener* listener)
	:StormService(loop, listener) {

}

int32_t EchoService::onRpcRequest(const Connection& conn, const RpcRequest& request, RpcResponse& response) {
	switch (request.proto_id()) {
		case 1:
		{
			EchoReq req;
			EchoAck ack;
			if (!req.ParseFromString(request.request())) {
				STORM_ERROR << "parse EchoReq error";
				// TODO
				return -1;
			}
			Echo(conn, req, ack);
			//if (request.invoke_type() != InvokeType_OneWay) {
				if (!ack.SerializeToString(response.mutable_response())) {
					STORM_ERROR << "serialize EchoAck error"; 
					return -1;
					//return RespStatus_CoderError;
				}
			//}
			break;
		}
		default:
			//return RespStatus_NoProtoId;
			return -1;
	}
	return 0;
}

static uint32_t count = 0;
static uint64_t lastTime = 0;
void EchoService::Echo(const Connection& conn, const EchoReq& req, EchoAck& ack) {
//	STORM_DEBUG << req.msg();
	ack.set_msg(req.msg());
	count++;
	uint64_t now = UtilTime::getNowMS();
	if (now - lastTime >= 3000) {
		STORM_DEBUG << count * 1.0 / (now - lastTime) * (1000);
		count = 0;
		lastTime = now;
	}
}

