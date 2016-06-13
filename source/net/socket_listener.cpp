#include "socket_listener.h"

#include "util/util_protocol.h"
#include "util/util_log.h"

namespace storm {

SocketListener::SocketListener(SocketLoop* loop)
	:m_loop(loop) {
	setPacketParser(PacketProtocolLen::decode);
}

bool SocketListener::onData(Socket* s) {
	IoBuffer* buffer = s->readBuffer;
	while (1) {
		const char* data = NULL;
		uint32_t len = 0;
		int32_t code = m_parser(buffer, data, len);
		if (code == Packet_Less) {
			return true;
		} else if (code == Packet_Error) {
			STORM_ERROR << "packet error, id: " << s->id << ", fd: " << s->fd << " size: " << buffer->getSize();
			return false;
		}
		onPacket(s, data, len);
	}
	return true;
}

}
