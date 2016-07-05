#ifndef _STORM_IO_BUFFER_H_
#define _STORM_IO_BUFFER_H_

#include <stdint.h>
#include <string>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <memory>

using namespace std;

namespace storm {

class IoBuffer {
public:
	typedef std::shared_ptr<IoBuffer> ptr;

	IoBuffer(uint32_t capacity = 4096)
		:m_capacity(capacity),
		 m_head(0),
		 m_tail(0) {
		m_data = (char*)malloc(sizeof(char) * m_capacity);
	}

	IoBuffer(const IoBuffer& rhs) {
		m_data = (char*)malloc(rhs.m_capacity);
		m_head = rhs.m_head;
		m_tail = rhs.m_tail;
		m_capacity = rhs.m_capacity;
		memcpy(m_data + m_head, rhs.m_data + m_head, m_tail - m_head);
	}

	IoBuffer(const string& str) {
		m_capacity = str.size();
		m_data = (char*)malloc(sizeof(char) * m_capacity);
		memcpy(m_data, str.c_str(), m_capacity);
		m_head = 0;
		m_tail = m_capacity;
	}

	IoBuffer(const char* data, uint32_t len) {
		m_capacity = len;
		m_data = (char*)malloc(sizeof(char) * m_capacity);
		memcpy(m_data, data, m_capacity);
		m_head = 0;
		m_tail = m_capacity;
	}

	IoBuffer& operator = (const IoBuffer& rhs) {
		m_data = (char*)malloc(rhs.m_capacity);
		m_head = rhs.m_head;
		m_tail = rhs.m_tail;
		m_capacity = rhs.m_capacity;
		memcpy(m_data + m_head, rhs.m_data + m_head, m_tail - m_head);
		return *this;
	}

	~IoBuffer() {
		free(m_data);
	}

	inline char* getHead() {
		return m_data + m_head;
	}

	inline char* getTail() {
		return m_data + m_tail;
	}

	inline uint32_t getSize() {
		return m_tail - m_head;
	}

	inline uint32_t getRemainingSize() {
		return m_capacity - m_tail;
	}

	inline uint32_t getAllRemaingSize() {
		return m_capacity - getSize();
	}

	inline uint32_t getCapacity() {
		return m_capacity;
	}

	inline void reset() {
		m_head = 0;
		m_tail = 0;
	}

	void readN(uint32_t iSize)
	{
		assert(iSize <= getSize());
		m_head += iSize;
		if (m_head == m_tail) {
			m_head = m_tail = 0;
		}
	}

	void writeN(uint32_t iSize)
	{
		assert(iSize <= getRemainingSize());
		uint32_t iDataSize = getSize() + iSize;
		m_tail += iSize;
		if (m_tail == m_capacity && m_head != 0){
			for (uint32_t i = 0; i < iDataSize; ++i) {
				m_data[i] = m_data[i + m_head];
			}
			m_head = 0;
			m_tail = iDataSize;
		}
	}

	void subTail(uint32_t size) {
		if (getSize() < size) {
			return;
		}
		m_tail -= size;
	}

	inline void doubleCapacity()
	{
		char* pData = (char*)malloc(sizeof(char) * m_capacity * 2);
		memcpy(pData, m_data, m_capacity);
		free(m_data);
		m_data = pData;
		m_capacity *= 2;
	}

	inline void reserve(uint32_t addSize) {
		while (getRemainingSize() < addSize) {
			doubleCapacity();
		}
	}

	//写数据
	void push_back(const string& str) {
		uint32_t dataSize = str.size();
		reserve(dataSize);
		memcpy(m_data + m_tail, str.c_str(), dataSize);
		m_tail += dataSize;
	}
	void push_back(const char* c, uint32_t len) {
		reserve(len);
		memcpy(m_data + m_tail, c, len);
		m_tail += len;
	}
	void push_back(uint32_t data) {
		reserve(4);
		for (uint32_t i = 0; i < 4; i++) {
			*(m_data + m_tail + (3 - i)) = (char)(data&0xFF);
			data >>= 8;
		}
		m_tail += 4;
	}
	void push_back(IoBuffer::ptr buffer) {
		reserve(buffer->getSize());
		memcpy(m_data + m_tail, buffer->getHead(), buffer->getSize());
		m_tail += buffer->getSize();
	}

private:
	char* m_data;
	uint32_t m_capacity;
	uint32_t m_head;
	uint32_t m_tail;
};
}

#endif
