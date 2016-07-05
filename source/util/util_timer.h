#ifndef _STORM_UTIL_TIMER_H_
#define _STORM_UTIL_TIMER_H_

#include <functional>
#include <map>
#include <unordered_map>
#include <atomic>

#include "util_time.h"

namespace storm {

class Timer {
public:
	Timer():m_id(0) {}

	typedef std::function<void ()> TimerCb;

	uint32_t addTimer(uint64_t ms, TimerCb cb, bool repeat = false);
	uint32_t addSecondTimer(uint32_t sec, TimerCb cb, bool repeat = false);
	bool delTimer(uint32_t id);
	void update(uint64_t ms);

private:
	struct TimerNode {
		uint32_t id;
		uint64_t expire;
		uint64_t duration;
		bool repeat;
		TimerCb cb;
	};

	typedef std::multimap<uint64_t, TimerNode*> TimerNodeMap;
	typedef std::unordered_map<uint32_t, TimerNode*> TimerHashMap;

	TimerNodeMap m_nodes;
	TimerHashMap m_search;
	atomic_uint m_id;
};
}

#endif
