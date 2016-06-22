#ifndef CONNECTION_H_
#define CONNECTION_H_

namespace storm {

struct Connection {
	Connection():id(0),fd(0),port(0) {}
	Connection(int32_t _id, int32_t _fd, string _ip, int32_t _port)
		:id(_id), fd(_fd), port(_port), ip(_ip) {}
	int32_t id;
	int32_t fd;
	int32_t port;
	string ip;
};

}

#endif
