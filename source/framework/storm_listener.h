#ifndef STORM_LISTENER_H_
#define STORM_LISTENER_H_

#include "net/socket_loop.h"
#include "net/socket_handler.h"

#include "util/util_misc.h"
#include "util/util_thread.h"
#include "util/util_timelist.h"

#include "connection.h"
#include "server_config.h"

namespace storm {
class StormService;
class StormListener : public SocketHandler {
public:
	friend class StormService;
	StormListener(SocketLoop* loop, bool inLoop = true);
	virtual ~StormListener(){}

	virtual void onAccept(Socket* s);
	virtual void onClose(Socket* s, uint32_t closeType);
	virtual void onPacket(Socket* s, const char* data, uint32_t len);
	
	int32_t startListen();

	// 逻辑线程定时调用
	void updateLogic();

	// 网络线程定时调用
	void updateNet();

	void doTimeClose(uint32_t id);
	void doEmptyClose(uint32_t id);

	uint32_t getOnlinNum() {
		return m_timelist.size();
	}


	SETTER(Service, StormService*, m_service);
	SETTER(InLoop, bool, m_inLoop);
	GETTER(InLoop, bool, m_inLoop);

	SETTER_REF(Name, std::string, m_name);
	GETTER_REF(Name, std::string, m_name);
	SETTER_REF(Ip, std::string, m_ip);
	SETTER(Port, int32_t, m_port);

public:
	enum PacketType {
		PacketType_Packet = 1,
		PacketType_Close = 2,
		PacketType_Accept = 3,
	};

	struct Packet {
		Packet(int32_t id, int32_t fd, string ip, int32_t port)
			:conn(id, fd, ip, port) {}
		Connection conn;
		int32_t type;
		int32_t closeType;
		string data;
	};

public:
	bool m_inLoop;					// 逻辑是否在loop线程处理
	StormService* m_service;		// 服务(具体逻辑处理器)
	std::string m_name;
	std::string m_ip;
	int32_t m_port;
	ServiceConfig m_config;
	LockQueue<Packet*> m_queue;
	TimeList<uint32_t, uint32_t> m_conList;  //新连接检测
	TimeList<uint32_t, uint32_t> m_timelist; //超时检测
};

}

#endif
