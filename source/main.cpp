#include <stdio.h>

#include "net/socket_loop.h"

#include "handler_test.h"

using namespace storm;

int main(int argc, char** argv) {
	printf("storm net new start!\n");

	SocketLoop loop;
	SocketListener listener(&loop);
	SocketConnector connector(&loop);
	loop.listen(&listener, "127.0.0.1", 1234);
	loop.listen(&listener, "", 4567);
	loop.connect(&connector, "127.0.0.1", 1234);
	loop.run();
	return 0;
}
