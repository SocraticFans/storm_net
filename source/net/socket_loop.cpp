#include "socket_loop.h"

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
#include <sys/eventfd.h>
#include <unistd.h>

#include "util/util_time.h"
#include "socket_util.h"

namespace storm {

SocketLoop::SocketLoop(uint32_t maxSocket)
	:m_running(false),
	 m_maxSocket(maxSocket),
	 m_allocId(-1) {

	m_slot.resize(m_maxSocket);

	// 通知的socket
	int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
	if (fd < 0) {
		//STORM_ERROR << "efd < 0";
		::exit(1);
	}
	m_notifier = getNewSocket();
	m_notifier->fd = fd;
	m_notifier->type = SocketType_Notify;
	m_notifier->status = SocketStatus_Listen;

	m_poll.addToRead(fd, m_notifier);
}

void SocketLoop::destroy() {
	// 关闭所有
    for (uint32_t i = 0; i < m_maxSocket; ++i) {
		Socket* s = &m_slot[i];
		forceClose(s, CloseType_Self);
    }
}

Socket* SocketLoop::getSocket(int32_t id) {
	if (id < 0) {
		return NULL;
	}
	return &m_slot[hash(id)];
}

Socket* SocketLoop::getNewSocket() {
	for (int32_t i = 0; i < (int32_t)m_maxSocket; ++i) {
		int32_t id = __sync_add_and_fetch(&m_allocId, 1);
		if (id < 0) {
			id = __sync_and_and_fetch(&m_allocId, 0x7fffffff);
		}
		Socket* s = &m_slot[hash(id)];
		if (s->status != SocketStatus_Idle) {
			continue;
		}
		if (__sync_bool_compare_and_swap(&s->status, SocketStatus_Idle, SocketStatus_Reserse)) {
			s->id = hash(id);
			s->fd = -1;
			return s;
		}
		--i;
	}
	return NULL;
}

void SocketLoop::forceClose(Socket* s, uint32_t closeType) {
	if (s->status == SocketStatus_Idle || s->status == SocketStatus_Reserse) {
		return;
	}

	// 回调
	if (s->handler) {
		s->handler->onClose(s, closeType);
	}

	// 清理读写缓存
	if (s->readBuffer) {
		s->readBuffer->reset();
		m_bufferPool.put(s->readBuffer);
		s->readBuffer = NULL;
	}
	for (list<IoBuffer*>::iterator it = s->writeBuffer.begin(); it != s->writeBuffer.end(); ) {
		IoBuffer* buffer = *it;
		delete buffer;
		s->writeBuffer.erase(it++);
	}

	m_poll.del(s->fd);
	::close(s->fd);
	s->status = SocketStatus_Idle;
}

void SocketLoop::handleAccept(Socket* s) {
	if (s->type != SocketType_Listen) {
		return;
	}
	sockaddr_in sa;
	socklen_t len = sizeof(sa);
	while (1) {
		int fd = ::accept(s->fd, (sockaddr*)&sa, &len);
		if (fd < 0) {
			if (errno == EAGAIN) {
				break;
			} else {
				continue;
			}
		}

		Socket* ns = getNewSocket();
		if (ns == NULL) {
			//STORM_ERROR << "over max socket num";
			::close(fd);
			break;
		}
		ns->fd = fd;
		ns->type = SocketType_Normal;
		ns->status = SocketStatus_Connected;
		ns->handler = s->handler;

    	socketKeepAlive(fd);
		socketNonBlock(fd);
		m_poll.addToRead(fd, ns);

		inet_ntop(AF_INET, (const void*)&sa.sin_addr, m_buffer, sizeof(m_buffer));
		m_buffer[MAX_INFO-1] = '\0';

		ns->ip = m_buffer;
		ns->port = ntohs(sa.sin_port);

		// 回调
		ns->handler->onAccept(ns);
	}
}

void SocketLoop::handleConnect(Socket* s) {
	int32_t error;
	socklen_t len = sizeof(error);
	int32_t code = getsockopt(s->fd, SOL_SOCKET, SO_ERROR, &error, &len);
	if (code < 0 || error) {
		//STORM_ERROR << "connect error: " << strerror(iError);
        forceClose(s, CloseType_ConnectFail);
		return;
	}
	s->status = SocketStatus_Connected;
	m_poll.delEvent(s->fd, EP_WRITE, s);

	s->handler->onConnect(s);
	m_poll.addEvent(s->fd, EP_READ, s);

	// TODO 这里应该不要，连接上发送数据，应该是更上层的设计
	//handleWrite(s);
}

void SocketLoop::handleRead(Socket* s) {
	if (s->status != SocketStatus_Connected) {
		return;
	}
	if (!s->readBuffer) {
		s->readBuffer = m_bufferPool.get();
	}
	IoBuffer* buffer = s->readBuffer;

	bool needClose = false;
	while (1) {
		uint32_t maxSize = buffer->getRemainingSize();
		//缓存满了，2倍扩充
		if (maxSize == 0) {
			buffer->doubleCapacity();
		}
		maxSize = buffer->getRemainingSize();
		int32_t readSize = ::read(s->fd, buffer->getTail(), maxSize);
		if (readSize == 0) {
			needClose = true;
			break;
		} else if (readSize < 0) {
    		if (errno == EAGAIN) { //没数据可以读
    			break;
    		} else if (errno == EINTR) {
    			continue;
    		} else {
				//STORM_ERROR << "connection error fd: " << s->fd << " error: " << strerror(errno);
				needClose = true;
    			break;
    		}
		} else {
			buffer->writeN(readSize);
		}
	}

	if (needClose) {
		forceClose(s, CloseType_Peer);
		return;
	}

	s->handler->onData(s);
}

void SocketLoop::handleWrite(Socket* s) {
	if (s->status != SocketStatus_Connected) {
		return;
	}
	char* d = NULL;
	int size = 0;
	int sendSize = 0;
	bool needClose = false;

	while (1) {
		if (s->writeBuffer.empty()) {
			m_poll.delEvent(s->fd, EP_WRITE, s);
			break;
		}
		IoBuffer* buffer = s->writeBuffer.front();
		d = buffer->getHead();
		size = buffer->getSize();

		sendSize = ::write(s->fd, d, size);
		if (sendSize == 0) {
			needClose = true;
			break;
		} else if (sendSize < 0) {
			if (errno == EAGAIN) {	//不能再写了
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				needClose = true;
				//STORM_INFO << "send error fd: " << s->fd << ", error: " << strerror(errno);
				break;
			}
		} else {
			buffer->readN(sendSize);
			if (buffer->getSize() == 0) {
				delete buffer;
				s->writeBuffer.pop_front();
			}
		}
	}

	if (needClose) {
		forceClose(s, CloseType_Peer);
	}
}

void SocketLoop::runOnce(int32_t ms) {
	int32_t num = m_poll.wait(m_event, MAX_EVENT, ms);
	for (int32_t i = 0; i < num; ++i) {
		SocketEvent* event = &m_event[i];
		Socket* s = (Socket*)(event->pUd);
		switch (s->type) {
			case SocketType_Notify:
				handleCmd();
				break;
			case SocketType_Listen:
				handleAccept(s);
				break;
			default:
			{
				if (s->status == SocketStatus_Connecting) {
					handleConnect(s);
				} else {
					if (event->bRead) {
						handleRead(s);
					}
					if (event->bWrite) {
						handleWrite(s);
					}
				}
				break;
			}
		}
	}
}

void SocketLoop::run() {
	m_running = true;
	while (m_running) {
		runOnce(2);
	}
	destroy();
}

void SocketLoop::handleCmd() {
	uint64_t d = 0;
	::read(m_notifier->fd, &d, sizeof(d));

	SocketCmd cmd;
	while (m_queue.pop_front(cmd, 0)) {
		switch(cmd.type) {
			case SocketCmd_Connect:
				connectSocket(cmd.id);
				break;
			case SocketCmd_Listen:
				listenSocket(cmd.id);
				break;
			case SocketCmd_Send:
				sendSocket(cmd.id, cmd.buffer);
				break;
			case SocketCmd_Close:
				closeSocket(cmd.id, cmd.closeType);
				break;
			case SocketCmd_Exit:
				m_running = false;
				break;
		}
	}
}

void SocketLoop::connectSocket(int32_t id) {
	Socket* s = getSocket(id);
	if (s == NULL) {
		return;
	}
	bool success = false;
	do {
		int fd = ::socket(AF_INET, SOCK_STREAM, 0);
		if (fd < 0) {
			break;
		}
		socketKeepAlive(fd);
		socketNonBlock(fd);

		//STORM_DEBUG << "ip " << s->ip << " port " << s->port;
		struct sockaddr_in stAddr;
		memset(&stAddr, 0, sizeof(stAddr));
		stAddr.sin_family = AF_INET;
		stAddr.sin_port = htons(s->port);
		inet_aton(s->ip.c_str(), &stAddr.sin_addr);

		int status = ::connect(fd, (struct sockaddr*)&stAddr, sizeof(stAddr));
		if (status != 0 && errno != EINPROGRESS) {
			::close(fd);
			//STORM_ERROR << "connect error: " << strerror(errno);
			break;
		}

		success = true;

		s->fd = fd;

		if (status == 0) {
			//连接成功
			s->status = SocketStatus_Connected;
			s->handler->onConnect(s);
			m_poll.addEvent(fd, EP_READ, s);
		} else {
			s->status = SocketStatus_Connecting;
			m_poll.addEvent(fd, EP_WRITE, s);
		}
	} while (0);

	if (!success) {
		//STORM_ERROR << "socket-server: connect socket error";
		s->handler->onClose(s, CloseType_ConnectFail);
		s->status = SocketStatus_Idle;
	}
}

void SocketLoop::listenSocket(int32_t id) {
	Socket* s = getSocket(id);
	if (s == NULL) {
		return;
	}
	m_poll.addToRead(s->fd, s);
}

void SocketLoop::sendSocket(int32_t id, IoBuffer* buffer) {
	Socket* s = getSocket(id);
	if (s == NULL || s->status != SocketStatus_Connected) {
		return;
	}
	if (!s->writeBuffer.empty()) {
		s->writeBuffer.push_back(buffer);
		return;
	}

	char* sendBuf = buffer->getHead();
	int remainSize = buffer->getSize();
	bool needClose = false;

	while (1) {
		if (remainSize == 0) {
			break;
		}
		int sendSize = ::write(s->fd, sendBuf, remainSize);
		if (sendSize == 0) {
			needClose = true;
			break;
		} else if (sendSize < 0) {
			if (errno == EAGAIN) {	//不能再写了
				m_poll.addToWrite(s->fd, s, true);
				break;
			} else if (errno == EINTR) {
				continue;
			} else {
				//STORM_INFO << "send error fd: " << s->fd << ", error: " << strerror(errno);
				needClose = true;
				break;
			}
		} else {
			sendBuf += sendSize;
			remainSize -= sendSize;
			buffer->readN(sendSize);
		}
	}
	if (needClose) {
		forceClose(s, CloseType_Peer);
		delete buffer;
	} else if (remainSize != 0) {
		s->writeBuffer.push_back(buffer);
	} else {
		delete buffer;
	}
}

void SocketLoop::closeSocket(int id, uint32_t closeType) {
	Socket* s = getSocket(id);
	if (s == NULL) {
		return;
	}
	forceClose(s, closeType);
}

inline void SocketLoop::pushCmd(const SocketCmd& cmd) {
	m_queue.push_back(cmd);
	uint64_t d = 1;
	::write(m_notifier->fd, &d, sizeof(d));
}

int32_t SocketLoop::connect(SocketHandler* handler, const string& ip, int32_t port) {
	Socket* s = getNewSocket();
	if (s == NULL) {
		return -1;
	}
	s->ip = ip;
	s->port = port;
	s->handler = handler;

	SocketCmd cmd;
	cmd.type = SocketCmd_Connect;
	cmd.id = s->id;
	pushCmd(cmd);

	return s->id;
}

int32_t SocketLoop::listen(SocketHandler* handler, const string& ip, int32_t port, int32_t backlog) {
	int32_t fd = socketListen(ip.c_str(), port, backlog);
	if (fd < 0) {
		//STORM_ERROR << "error when bind " << listenerName;
		return -1;
	}
	Socket* s = getNewSocket();
	if (s == NULL) {
		//STORM_ERROR << "error when getSocket " << listenerName;
		::close(fd);
		return -1;
	}

	s->fd = fd;
	s->type = SocketType_Listen;
	s->status = SocketStatus_Listen;
	s->handler = handler;

	SocketCmd cmd;
	cmd.type = SocketCmd_Listen;
	cmd.id = s->id;
	pushCmd(cmd);

	return s->id;
}

void SocketLoop::send(int32_t id, const string& data) {
	SocketCmd cmd;
	cmd.type = SocketCmd_Send;
	cmd.id = id;
	cmd.buffer = new IoBuffer(data);
	pushCmd(cmd);
}

void SocketLoop::close(int id, uint32_t closeType) {
	SocketCmd cmd;
	cmd.id = id;
	cmd.type = SocketCmd_Close;
	cmd.closeType = closeType;
	pushCmd(cmd);
}

void SocketLoop::terminate() {
	SocketCmd cmd;
	cmd.type = SocketCmd_Exit;
	pushCmd(cmd);
}

} // namespace storm


