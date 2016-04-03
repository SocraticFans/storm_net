#ifndef _UTIL_MISC_H_
#define _UTIL_MISC_H_

namespace storm {

#define LIKELY(x) __builtin_expect((x), 1)
#define UNLIKELY(x) __builtin_expect((x), 0)

class noncopyable {
protected:
	noncopyable() {}
	~noncopyable() {}
private:
	noncopyable(const noncopyable&);
	const noncopyable& operator= (const noncopyable&);
};

template <typename T>
class ObjectPool {
public:
	typedef std::list<T*> ContainerType;
	ObjectPool(uint32_t nodeNum = 128)
		:m_nodeNum(nodeNum) {
			expandNode();
	}

	~ObjectPool() {
		for (typename ContainerType::iterator it = m_freeNode.begin(); it != m_freeNode.end();) {
			T* node = *it;
			delete node;
			m_freeNode.erase(it++);
		}
	}

	T* get() {
		if (m_freeNode.empty()) {
			expandNode();
		}
		T* node = m_freeNode.front();
		m_freeNode.pop_front();
		return node;
	}

	void put(T* node) {
		m_freeNode.push_back(node);
	}

private:
	void expandNode() {
		for (uint32_t i = 0; i < m_nodeNum; ++i) {
			T* node = new T;
			m_freeNode.push_back(node);
		}
	}

private:
	uint32_t m_nodeNum;
	ContainerType m_freeNode;
};

}

#endif
