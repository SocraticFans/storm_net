#ifndef SOCKET_LOOP_H
#define SOCKET_LOOP_H

#include <vector>
#include "socket_define.h"
#include "socket_poller.h"
#include "util/util_thread.h"
#include "util/util_misc.h"

using namespace std;
namespace storm {

class SocketLoop {
public:
	SocketLoop(uint32_t maxSocket = 1024);

	int32_t connect(SocketHandler* h, const string& ip, int32_t port);
	int32_t listen(SocketHandler* h, const string& ip, int32_t port, int32_t backlog = 1024);
	void send(int32_t id, const string& data);
	void close(int id, uint32_t closeType = CloseType_Self);
	void terminate();

	void run();
	void runOnce(int32_t ms);

private:
	Socket* getSocket(int32_t id);
	Socket* getNewSocket();
	inline int32_t hash(int32_t id) {
		return id % m_maxSocket;
	}
	void forceClose(Socket* s, uint32_t closeType);
	inline void pushCmd(const SocketCmd& cmd);
	void destroy();

	// 事件处理
	void handleAccept(Socket* s);
	void handleConnect(Socket* s);
	void handleRead(Socket* s);
	void handleWrite(Socket* s);

	// 请求处理
	void handleCmd();
	void connectSocket(int32_t id);
	void listenSocket(int32_t id);
	void sendSocket(int32_t id, IoBuffer* buffer);
	void closeSocket(int32_t id, uint32_t closeType);

private:
	static const uint32_t MAX_EVENT = 1024;
	static const uint32_t MAX_INFO = 128;

	bool m_running;
	uint32_t m_maxSocket;				// 最大允许socket数目
	int32_t m_allocId;					// 分配id
	SocketPoller m_poll;				// epoll
	Socket* m_notifier;					// 唤醒epoll的socket
	LockQueue<SocketCmd> m_queue;		// 命令队列
	vector<Socket> m_slot;				// socket池
	SocketEvent m_event[MAX_EVENT];		// 事件
    char m_buffer[MAX_INFO];			// 存ip用的临时buffer
	ObjectPool<IoBuffer> m_bufferPool;	// 读缓存池
};

}

#endif
