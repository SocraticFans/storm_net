#include <stdio.h>

#include <unistd.h>
#include "framework/storm_server.h"

#include "EchoService.h"

using namespace storm;

class Client : public StormServer {
public:
	virtual bool init();
	virtual void destroy() {}
	virtual void loop();
};


EchoServiceProxy* g_proxy = NULL;

// 考虑下在init里调用其他服务，尤其是同步调用问题
bool Client::init() {
	g_proxy = m_proxyMgr->stringToProxy<EchoServiceProxy>("Server@tcp -h 127.0.0.1 -p 10002");
	return true;
}

void Client::loop() {
	EchoReq req; 
	EchoAck ack;
	while (!isTerminate()) {
		req.set_msg("Hello");
//		g_proxy->Echo(req, ack);
		g_proxy->async_Echo(NULL, req);
	}
	//sleep(1);
}


Client g_client;

int main(int argc, char** argv) {
	return g_client.run(argc, argv);
}
