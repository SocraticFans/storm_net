#include "socket_poller.h"

#include <assert.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>


namespace storm {

SocketPoller::SocketPoller()
	:m_iEpollFd(-1) {
	m_iEpollFd = epoll_create(1024);
}

SocketPoller::~SocketPoller() {
	if (m_iEpollFd != -1) {
		close(m_iEpollFd);
	}
}

bool SocketPoller::addToRead(uint32_t iFd, void* pUd) {
	epoll_event stEv;
	stEv.events = EPOLLIN | EPOLLET;
	stEv.data.ptr = pUd;
	if (epoll_ctl(m_iEpollFd, EPOLL_CTL_ADD, iFd, &stEv) == -1) {
		return false;
	}
	return true;
}

void SocketPoller::addToWrite(uint32_t iFd, void* pUd, bool bEnable) {
	epoll_event stEv;
	stEv.events = EPOLLIN | (bEnable ? EPOLLOUT : 0) | EPOLLET;
	stEv.data.ptr = pUd;
	epoll_ctl(m_iEpollFd, EPOLL_CTL_MOD, iFd, &stEv);
}

void SocketPoller::del(uint32_t fd) {
	uint32_t& mask = m_fdEvent[fd];
	if (mask == EP_NONE) return;
	mask = EP_NONE;
	epoll_ctl(m_iEpollFd, EPOLL_CTL_DEL, fd, NULL);
}

void SocketPoller::addEvent(uint32_t fd, uint32_t e, void* ud) {
	uint32_t& mask = m_fdEvent[fd];
	int option = mask == EP_NONE ? EPOLL_CTL_ADD : EPOLL_CTL_MOD;
	mask |= e;

	epoll_event stEv;
	stEv.events = 0;

	if(mask & EP_READ) stEv.events |= EPOLLIN;
	if(mask & EP_WRITE) stEv.events |= EPOLLOUT;
	stEv.events |= EPOLLET;
	stEv.data.ptr = ud;

	epoll_ctl(m_iEpollFd, option, fd, &stEv);
}

void SocketPoller::delEvent(uint32_t fd, uint32_t e, void* ud) {
	uint32_t& mask = m_fdEvent[fd];
	if (mask == EP_NONE) return;
	mask = mask & (~e);

	struct epoll_event ee;
	ee.events = 0;
	if(mask & EP_READ) ee.events |= EPOLLIN;
	if(mask & EP_WRITE) ee.events |= EPOLLOUT;
	ee.events |= EPOLLET;
	ee.data.ptr = ud;

	int option = mask == EP_NONE ? EPOLL_CTL_DEL : EPOLL_CTL_MOD;

	epoll_ctl(m_iEpollFd, option, fd, &ee);
}

int32_t SocketPoller::wait(SocketEvent* vEvents, uint32_t iMax, int32_t iTime) {
	assert(iMax <= MAX_EVENT);
	int32_t iNum = epoll_wait(m_iEpollFd, m_vEvents, iMax, iTime);
	for (int32_t i = 0; i < iNum; ++i) {
		vEvents[i].pUd = m_vEvents[i].data.ptr;
		uint32_t iFlag = m_vEvents[i].events;
		vEvents[i].bRead = (iFlag & EPOLLIN);
		if (iFlag & EPOLLOUT || iFlag & EPOLLERR || iFlag & EPOLLHUP) {
			vEvents[i].bWrite = true;
		} else {
			vEvents[i].bWrite = false;
		}
	}

	return iNum;
}

}
