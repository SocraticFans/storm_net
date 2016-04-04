#ifndef UTIL_PROTOCOL_H_
#define UTIL_PROTOCOL_H_

#include "net/socket_define.h"

namespace storm {

class PacketProtocolLen {
public:
	/*
	static IOBuffer::ptr encode(const string& data) {
		uint32_t packetLen = data.size() + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		packet->push_back(data);
		return packet;
	}
	*/

	/*
	template <typename T>
	static IOBuffer::ptr encode(const T& pb) {
		uint32_t pbLen = pb.ByteSize();
		uint32_t packetLen = pbLen + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		if (!pb.SerializeToArray(packet->getHead() + 4, pbLen)) {
			return IOBuffer::ptr();
		}
		packet->writeN(pbLen);
		return packet;
	}
	*/

	/*
	static IOBuffer::ptr encode(IOBuffer::ptr buffer) {
		uint32_t packetLen =  buffer->getSize() + 4;
		IOBuffer::ptr packet(new IOBuffer(packetLen));
		packet->push_back(packetLen);
		packet->push_back(buffer);
		return packet;
	}
	*/

	template <uint32_t maxLen = 65535>
	static int32_t decode(IoBuffer* in, const char*& buffer, uint32_t& size) {
		uint32_t len = in->getSize();
		if (len < 4) {
			return Packet_Less;
		}
		char* data = in->getHead();
		uint32_t packetLen = 0;
		for (uint32_t i = 0; i < 4; i++) {
			uint32_t shift_bit = (3-i)*8;
			packetLen += (data[i]<<shift_bit)&(0xFF<<shift_bit);
		}
		if (packetLen > maxLen) {
			return Packet_Error;
		}
		if (len < packetLen) {
			return Packet_Less;
		}
		buffer = in->getHead() + 4;
		size = packetLen - 4;
		in->readN(packetLen);

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
