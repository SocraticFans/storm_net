#include "EchoService.h"

void EchoServiceProxyCallBack::dispatch(RequestMessage* message) {
	uint32_t protoId = message->req.proto_id();
	switch (protoId) {
		case 1:
		{
			EchoAck ack;
			int ret = decodeResponse(message, ack);
			callback_Echo(ret, ack);
			break;
		}
		default:
		{
			STORM_ERROR << "unkown protoId " << protoId;
		}
	}
	ServiceProxy::delRequest(message);
}

int EchoServiceProxy::Echo(const EchoReq& req, EchoAck& ack) {
	RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	req.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int ret = decodeResponse(message, ack);
	delRequest(message);

	return ret;
}

void EchoServiceProxy::async_Echo(EchoServiceProxyCallBack* cb, const EchoReq& req) {
	RequestMessage* message = newRequest(InvokeType_Async, cb);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	req.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}
