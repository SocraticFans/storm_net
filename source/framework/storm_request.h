#ifndef _STORM_REQUEST_H_
#define _STORM_REQUEST_H_

#include "net/socket_define.h"
#include "proto/rpc_proto.pb.h"
#include "util/util_thread.h"

namespace storm {

// 调用类型
enum InvokeType {
	InvokeType_Sync,		// 同步
	InvokeType_Async,		// 异步
	InvokeType_OneWay,		// 单向
};

// 响应状态
enum ResponseStatus {
	ResponseStatus_Ok = 0,				// 调用成功
	ResponseStatus_TimeOut = 30001,		// 调用超时
	ResponseStatus_NetError = 30002,	// 网络出错
	ResponseStatus_CoderError = 30003,	// 编解码出错
	ResponseStatus_NoProtoId = 30004,	// 协议出错
	ResponseStatus_Error = 30005,		// 未知错误
};

class ServiceProxyCallBack;
class ProxyEndPoint;
struct RequestMessage {
	RequestMessage()
	:invokeType(InvokeType_Sync)
	,ep(NULL)
	,status(ResponseStatus_TimeOut)
	,resp(NULL)
	,back(false)
	,cb(NULL)
	,threadId(0) {

	}

	uint32_t invokeType;
	ProxyEndPoint* ep;
	ResponseStatus status;

	RpcRequest req;
	RpcResponse* resp;

	bool back;					// 是否已经回复
	ServiceProxyCallBack* cb;	// 异步回调
	Notifier notifier;			// 同步唤醒器
	uint32_t threadId;			// 发起请求的线程id
};

}

#endif
