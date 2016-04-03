#ifndef _STORM_THREAD_UTIL_H_
#define _STORM_THREAD_UTIL_H_

#include <list>
#include <stdint.h>
#include <pthread.h>
#include <memory>
#include <functional>
#include "util_misc.h"

namespace storm
{
class Mutex : public noncopyable {
public:
	Mutex() {
		pthread_mutex_init(&m_mutex, NULL);
	}
	~Mutex() {
		pthread_mutex_destroy(&m_mutex);
	}

	void lock() {
		pthread_mutex_lock(&m_mutex);
	}

	void unlock() {
		pthread_mutex_unlock(&m_mutex);
	}
	friend class Notifier;

private:
	pthread_mutex_t m_mutex;
};

template <typename T>
class ScopeMutex : public noncopyable {
public:
	ScopeMutex(T& lock):m_lock(lock), m_bLocked(true) {
		m_lock.lock();
	}
	~ScopeMutex() {
		unlock();
	}

	void lock() {
		if (!m_bLocked) {
			m_lock.lock();
		}
	}

	void unlock() {
		if (m_bLocked) {
			m_lock.unlock();
		}
	}

private:
	T& 	m_lock;
	bool	m_bLocked;
};

class Notifier : public noncopyable {
public:
	Notifier();
	~Notifier();

	void lock() { m_lock.lock(); }
	void unlock() { m_lock.unlock(); }

	void signal();
	void broadcast();
	void wait();
	void timedwait(uint64_t ms);

private:
	pthread_cond_t	m_cond;
	Mutex 			m_lock;
};

class Thread : public noncopyable {
public:
	typedef std::shared_ptr<Thread> ptr;
	Thread():m_bRunning(false) {}
	void start(std::function<void (void) > entry);
	void join();

	pthread_t getThreadId() {
		return m_thread;
	}

private:
	static void* threadEntry(void* argc);
private:
	bool m_bRunning;
	std::function<void (void) > m_entry;
	pthread_t m_thread;
};

template <typename T>
class LockQueue {
public:
	bool pop_front(T& t, int64_t ms = -1) {
		ScopeMutex<Notifier> lock(m_notifier);
		if (m_list.empty())
		{
			if (ms == 0) {
				return false;
			} else if (ms < 0) {
				m_notifier.wait();
			} else {
				m_notifier.timedwait(ms);
			}
		}
		if (m_list.empty()) {
			return false;
		}
		std::swap(m_list.front(), t);
		m_list.pop_front();
		return true;
	}

	void push_back(const T &t)
	{
		ScopeMutex<Notifier> lock(m_notifier);
		m_list.push_back(t);
		m_notifier.signal();
	}

	void swap(std::list<T>& l)
	{
		ScopeMutex<Notifier> lock(m_notifier);
		std::swap(l, m_list);
	}

	uint32_t size()
	{
		ScopeMutex<Notifier> lock(m_notifier);
		return m_list.size();
	}

	bool empty()
	{
		ScopeMutex<Notifier> lock(m_notifier);
		return m_list.empty();
	}

	void wakeup()
	{
		ScopeMutex<Notifier> lock(m_notifier);
		m_notifier.broadcast();
	}
private:
	std::list<T>	m_list;
	Notifier		m_notifier;
};

uint32_t getTid();
const char* getThreadIdStr();
}
#endif
