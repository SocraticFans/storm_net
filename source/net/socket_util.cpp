#include "socket_util.h"

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>


namespace storm {

void socketNonBlock(int iFd) {
	int iFlag = fcntl(iFd, F_GETFL, 0);
	if (iFlag == -1) {
		return;
	}
	fcntl(iFd, F_SETFL, iFlag | O_NONBLOCK);
}

void socketKeepAlive(int iFd) {
	int keepalive = 1;
	setsockopt(iFd, SOL_SOCKET, SO_KEEPALIVE, (void*)&keepalive, sizeof(keepalive));
}

int socketBind(const char* pHost, int iPort) {
	if (pHost == NULL || pHost[0] == 0) {
		pHost = "0.0.0.0";
	}

    char pPortStr[16];
    sprintf(pPortStr, "%d", iPort);

    struct addrinfo ai_hints;
    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    struct addrinfo* ai_list = NULL;
	int iStatus;
    iStatus = getaddrinfo(pHost, pPortStr, &ai_hints, &ai_list);
    if (iStatus != 0) {
        return -1;
    }

    int iFd;
	bool bSuccess = false;
	do {
    	iFd = ::socket(ai_list->ai_family, ai_list->ai_socktype, 0);
		if (iFd < 0) {
			break;
		}
		int reuse = 1;
		if (setsockopt(iFd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)) == -1) {
			::close(iFd);
			break;
		}
		iStatus = ::bind(iFd, (struct sockaddr*)ai_list->ai_addr, ai_list->ai_addrlen);
		if (iStatus != 0) {
			::close(iFd);
			break;
		}
		bSuccess = true;
	} while (0);

    freeaddrinfo(ai_list);
	if (!bSuccess) {
//		STORM_ERROR << "socket-server error when binding: " << strerror(errno);
		return -1;
	}
    return iFd;
}

int socketListen(const char* pHost, int iPort, int iBacklog) {
	int iListenFD = socketBind(pHost, iPort);
	if (iListenFD < 0) {
		return -1;
	}
	socketNonBlock(iListenFD);
	if (::listen(iListenFD, iBacklog) == -1) {
        ::close(iListenFD);
        return -1;
	}
    return iListenFD;
}

}
