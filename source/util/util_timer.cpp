#include "util_timer.h"
#include <vector>

namespace storm {

uint32_t Timer::addTimer(uint64_t ms, TimerCb cb, bool repeat) {
	uint64_t nowMs = UtilTime::getNowMS();
	uint64_t expire = nowMs + ms;
	uint32_t id = m_id.fetch_add(1);

	TimerNode* node = new TimerNode;
	node->expire = expire;
	node->duration = ms;
	node->id = id;
	node->repeat = repeat;
	node->cb = cb;

	m_nodes.insert(std::make_pair(expire, node));
	m_search[id] = node;
	return id;
}

uint32_t Timer::addSecondTimer(uint32_t sec, TimerCb cb, bool repeat) {
	return addTimer(sec * 1000, cb, repeat);
}

bool Timer::delTimer(uint32_t id) {
	TimerHashMap::iterator it = m_search.find(id);
	if (it == m_search.end()) {
		return false;
	}
	bool found = false;
	TimerNode* node = it->second;
	auto itPair  = m_nodes.equal_range(node->expire);
	for (TimerNodeMap::iterator itNode = itPair.first; itNode != itPair.second; itNode++) {
		if (itNode->second->id == id) {
			found = true;
			m_nodes.erase(itNode);
			m_search.erase(it);
			delete node;
			break;
		}
	}

	return found;
}

void Timer::update(uint64_t ms) {
	std::vector<std::pair<uint64_t, TimerNode*> > newNodes;
	for (TimerNodeMap::iterator it = m_nodes.begin(); it != m_nodes.end(); ) {
		TimerNode* node = it->second;
		if (ms > node->expire) {
			break;
		}
		node->cb();
		if (node->repeat) {
			node->expire = ms + node->duration;
			m_nodes.erase(it++);
			newNodes.push_back(std::make_pair(node->expire, node));
		} else {
			m_nodes.erase(it++);
			m_search.erase(node->id);
			delete node;
		}
	}

	for (auto it = newNodes.begin(); it != newNodes.end(); ++it) {
		m_nodes.insert(*it);
	}
}

}
