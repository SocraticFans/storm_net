#include "storm_service.h"

#include <stdio.h>
#include <iostream>

#include "util/util_log.h"
#include "util/util_protocol.h"

#include "storm_listener.h"

namespace storm {

StormService::StormService(SocketLoop* loop, StormListener* listener)
	:m_loop(loop),
	 m_listener(listener),
	 m_running(false) {
	m_listener->setService(this);
}

void StormService::onRequest(const Connection& conn, const char* buffer, uint32_t len) {
	int32_t ret = 0;
	RpcRequest req;
	RpcResponse resp;

	// 解析RpcRequest
	if (!req.ParseFromArray(buffer, len)) {
		STORM_ERROR << "rpc request error";
		m_loop->close(conn.id, CloseType_PacketError);
		return;
	}

	// 业务逻辑
	ret = onRpcRequest(conn, req, resp);

	/*
	if (req.invoke_type() == InvokeType_OneWay) {
		return;
	}
	*/

	//回包
	resp.set_ret(ret);
	resp.set_proto_id(req.proto_id());
	resp.set_request_id(req.request_id());

	//IOBuffer::ptr respBuf = FrameProtocolLen::encode(resp);
	string data;
	m_loop->send(conn.id, data);
}

void StormService::startThread() {
	m_thread = std::thread(&StormService::loop, this);
}

void StormService::terminateThread() {
	m_running = false;
	// 唤醒工作线程(会多次唤醒)
	m_listener->m_queue.wakeup();
	m_thread.join();
	destroy();
}

void StormService::loop() {
	m_running = true;
	init();
	while (m_running) {
		update(-1);
	}
}

void StormService::update(int64_t ms) {
	StormListener::Packet* packet;
	while (m_listener->m_queue.pop_front(packet, ms)) {
		switch (packet->type) {
			case StormListener::PacketType_Accept:
				onAccept(packet->conn);
				break;
			case StormListener::PacketType_Close:
				onClose(packet->conn, packet->closeType);
				break;
			case StormListener::PacketType_Packet:
				onRequest(packet->conn, packet->data.c_str(), packet->data.size());
				break;
		}
		delete packet;
	}
}

}
