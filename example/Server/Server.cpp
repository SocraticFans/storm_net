#include <stdio.h>

#include "net/socket_loop.h"

#include "handler_test.h"
#include "util/util_protocol.h"
#include "storm_net.h"

#include "framework/storm_cmd_service.h"

#include "echo_service.h"

using namespace storm;

// 启动流程思考
// 提供register Service方法，注册Service
// 然后启动各个线程
// 然后init（）
// 这样init里可以对外网络请求

class Server : public StormServer {
public:
	virtual bool init();
	virtual void mainLoop() {};
	virtual void mainLoopDestory() {};
};

// 考虑下在init里调用其他服务，尤其是同步调用问题
bool Server::init() {
//	addService<StormService>("TestService", PacketProtocolLine::decode);
	addService<StormCmdService>("TestService");
	addService<EchoServiceImp>("EchoService");
	return true;
}

Server g_server;

int main(int argc, char** argv) {
	g_server.run(argc, argv);

	/*
	SocketLoop loop;

	// 监听器
	Listener listener(&loop);
	listener.setPacketParser(PacketProtocolLine::decode);
	Service service(&loop, &listener);
	loop.listen(&listener, "127.0.0.1", 1234);

	// 连接器
	SocketConnector connector(&loop);
	loop.connect(&connector, "127.0.0.1", 1234);

	loop.run();
	*/
	return 0;
}
