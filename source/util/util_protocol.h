#ifndef UTIL_PROTOCOL_H_
#define UTIL_PROTOCOL_H_

#include "net/socket_define.h"
#include <google/protobuf/message.h>

namespace storm {

class PacketProtocolLen {
public:
	static IoBuffer* encode(const string& data) {
		uint32_t bufferLen = data.size() + 4;
		IoBuffer* buffer = new IoBuffer(bufferLen);
		buffer->push_back(bufferLen);
		buffer->push_back(data);
		return buffer;
	}

	static IoBuffer* encode(const google::protobuf::Message& msg) {
		uint32_t len = msg.ByteSize();
		uint32_t bufferLen = len + 4;
		IoBuffer* buffer = new IoBuffer(bufferLen);
		buffer->push_back(bufferLen);
		if (!msg.SerializeToArray(buffer->getHead() + 4, len)) {
			return NULL;
		}
		buffer->writeN(len);
		return buffer;
	}

	/*
	static IoBuffer::ptr encode(IoBuffer::ptr buffer) {
		uint32_t bufferLen =  buffer->getSize() + 4;
		IoBuffer::ptr buffer(new IoBuffer(bufferLen));
		buffer->push_back(bufferLen);
		buffer->push_back(buffer);
		return buffer;
	}
	*/

	template <uint32_t maxLen = 65535>
	static int32_t decode(IoBuffer* in, const char*& buffer, uint32_t& size) {
		uint32_t len = in->getSize();
		if (len < 4) {
			return Packet_Less;
		}
		char* data = in->getHead();
		uint32_t bufferLen = 0;
		for (uint32_t i = 0; i < 4; i++) {
			uint32_t shift_bit = (3-i)*8;
			bufferLen += (data[i]<<shift_bit)&(0xFF<<shift_bit);
		}
		if (bufferLen > maxLen) {
			return Packet_Error;
		}
		if (len < bufferLen) {
			return Packet_Less;
		}
		buffer = in->getHead() + 4;
		size = bufferLen - 4;
		in->readN(bufferLen);

		return Packet_Normal;
	}
};

class PacketProtocolLine {
public:
	template <uint32_t maxLen = 10000>
	static int32_t decode(IoBuffer* in, const char*& buffer, uint32_t& size) {
		uint32_t len = in->getSize();
		if (len == 0) {
			return Packet_Less;
		}
		char* data = in->getHead();
		for (uint32_t i = 0; i < len - 1; ++i) {
			if (data[i] == '\r' && data[i+1] == '\n') {
				buffer = in->getHead();
				size = i;
				in->readN(i+2);
				return Packet_Normal;
			}
			if (i >= maxLen) {
				return Packet_Error;
			}
		}
		return Packet_Less;
	}
};

}

#endif
