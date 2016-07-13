/*代码生成器自动生成，请不要手动修改!*/
#include "echo.h"


using namespace storm;

int32_t EchoService::onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp) {
	int32_t ret = 0;
	switch (req.proto_id()) {
		case 1:
		{
			EchoReq __request;
			EchoAck __response;
			if (!__request.ParseFromString(req.request())) {
				STORM_ERROR << "error";
				return ResponseStatus_CoderError;
			}
			ret = Echo(conn, __request, __response);
			if (req.invoke_type() != InvokeType_OneWay) {
				if (!__response.SerializeToString(resp.mutable_response())) {
					STORM_ERROR << "error"; 
					return ResponseStatus_CoderError;
				}
			}
			break;
		}
		default:
			return ResponseStatus_NoProtoId;
	}

	return ret;
}

void EchoServiceProxyCallBack::dispatch(storm::RequestMessage* req) {
	uint32_t protoId = req->req.proto_id();
	switch (protoId) {
		case 1:
		{
			EchoAck __response;
			int32_t ret = decodeResponse(req, __response);
			callback_Echo(ret, __response);
			break;
		}
		default:
		{
			STORM_ERROR << "unkown protoId " << protoId;
		}
	}
	storm::ServiceProxy::delRequest(req);
}

int32_t EchoServiceProxy::Echo(const EchoReq& request, EchoAck& response) {
	storm::RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int32_t ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void EchoServiceProxy::async_Echo(EchoServiceProxyCallBack* cb, const EchoReq& request, bool broadcast) {
	storm::RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}
