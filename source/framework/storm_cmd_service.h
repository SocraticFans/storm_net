#ifndef STORM_CMD_SERVICE_H
#define STORM_CMD_SERVICE_H

#include <map>
#include <functional>

#include "storm_service.h"

namespace storm {

class StormCmdService : public StormService {
public:
	StormCmdService(SocketLoop* loop, StormListener* listener);
	virtual ~StormCmdService() {}

	virtual void onRequest(const Connection& conn, const char* data, uint32_t len);
	virtual void onClose(const Connection& conn, uint32_t closeType) {}
	virtual void onAccept(const Connection& conn) {}

	typedef std::function<int32_t (const Connection&, const vector<std::string>&, std::string&)> Handler;
	void registerHandler(const std::string& cmd, Handler handler);

private:
	int32_t ping(const Connection& conn, const std::vector<std::string>& params, std::string& out);
	int32_t status(const Connection& conn, const std::vector<std::string>& params, std::string& out);

private:
	typedef std::map<std::string, Handler> HandlerMap;
	HandlerMap m_handlers;
};
}
#endif
