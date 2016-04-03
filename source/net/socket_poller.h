#ifndef _STORM_SOCKET_POLLER_H_
#define _STORM_SOCKET_POLLER_H_

#include <stdint.h>
#include <sys/epoll.h>
#include <unordered_map>

namespace storm {

struct SocketEvent {
	void* pUd;
	bool bRead;
	bool bWrite;
};

enum {
	EP_NONE = 0,
	EP_READ = 1,
	EP_WRITE = 2,
};

class SocketPoller {
public:
	static const uint32_t MAX_EVENT = 1024;
	SocketPoller();
	~SocketPoller();

	bool addToRead(uint32_t iFd, void* pUd);
	void addToWrite(uint32_t iFd, void* pUd, bool bEnable);
	void del(uint32_t iFd);

	void addEvent(uint32_t fd, uint32_t e, void* ud);
	void delEvent(uint32_t fd, uint32_t e, void* ud);

	int32_t wait(SocketEvent* vEvents, uint32_t iMax, int32_t iTime = -1);

	inline bool invalid() {
		return m_iEpollFd == -1;
	}

	uint32_t getFd() {
		return m_iEpollFd;
	}

private:
	int32_t m_iEpollFd;
	epoll_event m_vEvents[MAX_EVENT];
	std::unordered_map<uint32_t, uint32_t> m_fdEvent;
};
}

#endif
