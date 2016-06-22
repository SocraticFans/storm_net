#ifndef _STORM_UTIL_SINGLETON_H_
#define _STORM_UTIL_SINGLETON_H_

#include <pthread.h>

namespace storm {
/**
 * 非线程安全Singleton
 */
template<typename T>
class Singleton {
public:
	static T* instance() {
		if (m_instance == NULL) {
			m_instance = new T();
		}
		return m_instance;
	}
protected:
	static T* m_instance;
};

template <typename T> T* Singleton<T>::m_instance = NULL;

/**
 * 线程Singleton
 */
template<class T>
class ThreadSingleton {
public:
	static T* instance() {
		pthread_once(&m_inited, init);
		T* t = (T*)pthread_getspecific(m_key);
		if (t == NULL) {
			t = new T();
			set(t);
		}
		return t;
	}

	static void del() {
		delete instance();
		set(NULL);
	}

private:
	static void set(T* t) {
		pthread_setspecific(m_key, t);
	}

	static void destructor(void* p) {
		if (p) {
			delete (T*)p;
		}
	}

	static void init() {
		pthread_key_create(&m_key, destructor);
	}

private:
	static pthread_key_t m_key;
	static pthread_once_t m_inited;
};

template <typename T> pthread_key_t ThreadSingleton<T>::m_key;
template <typename T> pthread_once_t ThreadSingleton<T>::m_inited = PTHREAD_ONCE_INIT;

/**
 * 线程安全Singleton
 */
template<class T>
class SafeSingleton {
public:
	static T* instance() {
		pthread_once(&m_inited, init);
		return m_instance;
	}

private:
	static void init() {
		m_instance = new T();
	}

private:
	static T* m_instance;
	static pthread_once_t m_inited;
};

template <typename T> T* SafeSingleton<T>::m_instance = NULL;
template <typename T> pthread_once_t SafeSingleton<T>::m_inited = PTHREAD_ONCE_INIT;

}
#endif
