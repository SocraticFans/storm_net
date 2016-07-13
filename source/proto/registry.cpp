/*代码生成器自动生成，请不要手动修改!*/
#include "registry.h"


using namespace storm;

int32_t RegistryService::onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp) {
	int32_t ret = 0;
	switch (req.proto_id()) {
		case 1:
		{
			QueryServiceReq __request;
			QueryServiceAck __response;
			if (!__request.ParseFromString(req.request())) {
				STORM_ERROR << "error";
				return ResponseStatus_CoderError;
			}
			ret = Query(conn, __request, __response);
			if (req.invoke_type() != InvokeType_OneWay) {
				if (!__response.SerializeToString(resp.mutable_response())) {
					STORM_ERROR << "error"; 
					return ResponseStatus_CoderError;
				}
			}
			break;
		}
		case 2:
		{
			ServiceHeartBeatReq __request;
			ServiceHeartBeatAck __response;
			if (!__request.ParseFromString(req.request())) {
				STORM_ERROR << "error";
				return ResponseStatus_CoderError;
			}
			ret = HeartBeat(conn, __request, __response);
			if (req.invoke_type() != InvokeType_OneWay) {
				if (!__response.SerializeToString(resp.mutable_response())) {
					STORM_ERROR << "error"; 
					return ResponseStatus_CoderError;
				}
			}
			break;
		}
		case 3:
		{
			ServiceStopReq __request;
			ServiceStopAck __response;
			if (!__request.ParseFromString(req.request())) {
				STORM_ERROR << "error";
				return ResponseStatus_CoderError;
			}
			ret = Stop(conn, __request, __response);
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

void RegistryServiceProxyCallBack::dispatch(storm::RequestMessage* req) {
	uint32_t protoId = req->req.proto_id();
	switch (protoId) {
		case 1:
		{
			QueryServiceAck __response;
			int32_t ret = decodeResponse(req, __response);
			callback_Query(ret, __response);
			break;
		}
		case 2:
		{
			ServiceHeartBeatAck __response;
			int32_t ret = decodeResponse(req, __response);
			callback_HeartBeat(ret, __response);
			break;
		}
		case 3:
		{
			ServiceStopAck __response;
			int32_t ret = decodeResponse(req, __response);
			callback_Stop(ret, __response);
			break;
		}
		default:
		{
			STORM_ERROR << "unkown protoId " << protoId;
		}
	}
	storm::ServiceProxy::delRequest(req);
}

int32_t RegistryServiceProxy::Query(const QueryServiceReq& request, QueryServiceAck& response) {
	storm::RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int32_t ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void RegistryServiceProxy::async_Query(RegistryServiceProxyCallBack* cb, const QueryServiceReq& request, bool broadcast) {
	storm::RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(1);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}

int32_t RegistryServiceProxy::HeartBeat(const ServiceHeartBeatReq& request, ServiceHeartBeatAck& response) {
	storm::RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(2);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int32_t ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void RegistryServiceProxy::async_HeartBeat(RegistryServiceProxyCallBack* cb, const ServiceHeartBeatReq& request, bool broadcast) {
	storm::RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(2);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}

int32_t RegistryServiceProxy::Stop(const ServiceStopReq& request, ServiceStopAck& response) {
	storm::RequestMessage* message = newRequest(InvokeType_Sync);
	message->req.set_proto_id(3);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);
	int32_t ret = decodeResponse(message, response);

	delRequest(message);
	return ret;
}

void RegistryServiceProxy::async_Stop(RegistryServiceProxyCallBack* cb, const ServiceStopReq& request, bool broadcast) {
	storm::RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);
	uint32_t invokeType = message->invokeType;
	message->req.set_proto_id(3);
	request.SerializeToString(message->req.mutable_request());

	doInvoke(message);

	if (invokeType == InvokeType_OneWay) {
		delRequest(message);
	}
}
