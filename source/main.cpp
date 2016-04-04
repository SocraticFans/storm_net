#include <stdio.h>

#include "net/socket_loop.h"

#include "handler_test.h"
#include "storm_net.h"
#include "util/util_protocol.h"

using namespace storm;

int main(int argc, char** argv) {
	printf("storm net new start!\n");

	SocketLoop loop;

	// 监听器
	Listener listener(&loop);
	listener.setPacketParser(std::bind(PacketProtocolLine::decode, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	Service service(&loop, &listener);
	loop.listen(&listener, "127.0.0.1", 1234);

	// 连接器
	SocketConnector connector(&loop);
	loop.connect(&connector, "127.0.0.1", 1234);

	loop.run();
	return 0;
}
