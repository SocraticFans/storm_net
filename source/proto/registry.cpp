/*代码生成器自动生成，请不要手动修改!*/
#include "registry.h"


int32_t RegistryService::onRpcRequest(const Connection& conn, const RpcRequest& req, RpcResponse& resp) {
	switch (req.proto_id()) {
		case 1:
		{
			QueryServiceReq __request;
			QueryServiceAck __response;
			if (!__request.ParseFromString(req.request())) {
				STORM_ERROR << "error";
				return ResponseStatus_CoderError;
			}
			Query(conn, __request, __response);
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

	return 0;
}

void RegistryServiceProxyCallBack::dispatch(RequestMessage* req) {
	uint32_t protoId = req->req.proto_id();
	switch (protoId) {
		case 1:
		{
			QueryServiceAck __response;
			int32_t ret = decodeResponse(req, __response);
			callback_Query(ret, __response);
			break;
		}
		default:
		{
			STORM_ERROR << "unkown protoId " << protoId;
		}
	}
	ServiceProxy::delRequest(req);
}

int32_t RegistryServiceProxy::Query(const QueryServiceReq& request, QueryServiceAck& response) {
	RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int32_t ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void RegistryServiceProxy::async_Query(RegistryServiceProxyCallBack* cb, const QueryServiceReq& request, bool broadcast) {
	RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}
