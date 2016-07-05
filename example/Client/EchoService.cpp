#include "EchoService.h"

int EchoServiceProxy::Echo(const EchoReq& req, EchoAck& ack) {
	RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	req.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	//int ret = decodeResponse(message, response);

	//delRequest(message);
	//return ret;
	return 0;
}

void EchoServiceProxy::async_Echo(ServiceProxyCallBack* cb, const EchoReq& req) {
	RequestMessage* message = newRequest(InvokeType_Async, cb);
//	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	req.SerializeToString(message->req.mutable_request());

	doInvoke(message);

//	if (invokeType == InvokeType_OneWay) {
//		delRequest(message);
//	}
}
