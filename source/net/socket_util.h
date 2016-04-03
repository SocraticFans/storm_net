#ifndef _STORM_SOCKET_UTIL_H_
#define _STORM_SOCKET_UTIL_H_

#include <fcntl.h>
#include <stdint.h>
#include <sys/socket.h>

namespace storm {

	void socketNonBlock(int iFd);

	void socketKeepAlive(int iFd);

	int socketBind(const char* pHost, int iPort);

	int socketListen(const char* pHost, int iPort, int iBacklog);

}

#endif
