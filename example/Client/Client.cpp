#include <stdio.h>

#include <unistd.h>
#include "framework/storm_server.h"

#include "util/util_time.h"
#include "echo.h"


using namespace storm;

class Client : public StormServer {
public:
	virtual bool init();
	virtual void mainLoop();
	virtual void netLoop();
};


EchoServiceProxy* g_proxy = NULL;

// 考虑下在init里调用其他服务，尤其是同步调用问题
bool Client::init() {
	g_proxy = m_proxyMgr->stringToProxy<EchoServiceProxy>("Server@tcp -h 127.0.0.1 -p 10002");
	setLoopInterval(2000);
	return true;
}

int64_t last = 0;

class CallBackEcho : public EchoServiceProxyCallBack {
public:
	virtual void callback_Echo(int32_t ret, const EchoAck& ack) {
		STORM_DEBUG << "ret: " << ret << ", ack: " << ack.Utf8DebugString();
	}
};

void Client::mainLoop() {
	EchoReq req; 
	EchoAck ack;
	//while (!isTerminate()) {
		req.set_msg("Hello");
		int32_t ret = g_proxy->Echo(req, ack);
		if (ret == 0) {
			STORM_DEBUG << "ack: " << ack.Utf8DebugString();
		}
		//g_proxy->async_Echo(new CallBackEcho, req);
		sleep(1);
	//}
}

void Client::netLoop() {
	EchoReq req; 
	req.set_msg("Hello");
	//g_proxy->async_Echo(new CallBackEcho, req);
	//sleep(1);
}


Client g_client;

int main(int argc, char** argv) {
	return g_client.run(argc, argv);
}
