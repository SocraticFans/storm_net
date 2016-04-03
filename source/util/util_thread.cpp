#include "util_thread.h"

#include <unistd.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <time.h>
#include <thread>

#include "util_time.h"

namespace storm {
Notifier::Notifier() {
	pthread_cond_init(&m_cond, NULL);
}

Notifier::~Notifier() {
	pthread_cond_destroy(&m_cond);
}

void Notifier::signal() {
	pthread_cond_signal(&m_cond);
}

void Notifier::broadcast() {
	pthread_cond_broadcast(&m_cond);
}

void Notifier::wait() {
	pthread_cond_wait(&m_cond, &m_lock.m_mutex);
}

void Notifier::timedwait(uint64_t ms) {
	ms += UtilTime::getNowMS();
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (ms % 1000) * 1000 * 1000;
	pthread_cond_timedwait(&m_cond, &m_lock.m_mutex, &ts);
}

void* Thread::threadEntry(void* argc) {
	Thread* pThread = static_cast<Thread*>(argc);
	pthread_detach(pthread_self());
	pThread->m_entry();

	return (void*)NULL;
}

void Thread::start(std::function<void (void) > entry) {
	if (m_bRunning) {
		return;
	}
	m_entry = entry;
	m_bRunning = true;
	pthread_create(&m_thread, NULL, threadEntry, this);
}

void Thread::join() {
	if (!m_bRunning) {
		return;
	}
	pthread_join(m_thread, NULL);
}

__thread uint32_t t_tid = 0;
__thread char t_tidString[32];

static pid_t gettid() {      
	return syscall(SYS_gettid); 
} 

uint32_t getTid() {
	if (UNLIKELY(t_tid == 0)) {
		t_tid = gettid();
		snprintf(t_tidString, sizeof t_tidString, "%5d", t_tid);
	}
	return t_tid;
}

const char* getThreadIdStr() {
	if (UNLIKELY(t_tid == 0)) {
		t_tid = gettid();
		snprintf(t_tidString, sizeof t_tidString, "%d", t_tid);
	}
	return t_tidString;
}

}
