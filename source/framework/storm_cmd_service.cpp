#include "storm_cmd_service.h"

#include <stdio.h>
#include <iostream>

#include "util/util_log.h"
#include "util/util_protocol.h"
#include "util/util_string.h"

#include "storm_listener.h"
#include "storm_server.h"

using namespace std::placeholders;

namespace storm {

StormCmdService::StormCmdService(SocketLoop* loop, StormListener* listener)
 :StormService(loop, listener) {
	m_listener->setPacketParser(PacketProtocolLine::decode);

	registerHandler("ping", std::bind(&StormCmdService::ping, this, _1, _2, _3));
	registerHandler("status", std::bind(&StormCmdService::status, this, _1, _2, _3));
}

void StormCmdService::onRequest(const Connection& conn, const char* buffer, uint32_t len) {
	string data(buffer, len);
	std::vector<std::string> params = UtilString::splitString(data, " ");

	if (params.empty()) {
		return;
	}
	std::string cmd = params[0];
	auto it = m_handlers.find(cmd);
	if (it == m_handlers.end()) {
		m_loop->send(conn.id, "invalid cmd: " + cmd + "\n");
		return;
	}
	params.erase(params.begin());
	string out;
	(it->second)(conn, params, out);
	m_loop->send(conn.id, out + "\n");
}

void StormCmdService::registerHandler(const std::string& cmd, Handler handler) {
	auto it = m_handlers.find(cmd);
	if (it != m_handlers.end()) {
		STORM_ERROR << "duplicate cmd: " << cmd;
	}
	m_handlers[cmd] = handler;
}

int32_t StormCmdService::ping(const Connection& conn, const std::vector<std::string>& params, std::string& out) {
	out = "pong, params size: " + UtilString::tostr(params.size());
	return 0;
}

int32_t StormCmdService::status(const Connection& conn, const std::vector<std::string>& params, std::string& out) {
	g_stormServer->status(out);
	return 0;
}

}
