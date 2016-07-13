#include "echo_service.h"
#include "util/util_time.h"

static uint32_t count = 0;
static uint64_t lastTime = 0;
int32_t EchoServiceImp::Echo(const Connection& conn, const EchoReq& req, EchoAck& ack) {
//	STORM_DEBUG << req.msg();
	ack.set_msg(req.msg());
	count++;
	uint64_t now = UtilTime::getNowMS();
	if (now - lastTime >= 3000) {
		STORM_DEBUG << count * 1.0 / (now - lastTime) * (1000);
		count = 0;
		lastTime = now;
	}
	return 0;
}

