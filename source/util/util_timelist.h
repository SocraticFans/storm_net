#ifndef _TIME_LIST_H_
#define _TIME_LIST_H_

#include <unordered_map>
#include <list>
#include <functional>

#include <stdio.h>
#include "util_thread.h"

using namespace std;

namespace storm {

template <typename K, typename TimeType>
class TimeList {
public:
	class MapData;
	class ListData;

	typedef unordered_map<K, MapData> map_type;
	typedef list<ListData> list_type;

	struct MapData {
		typename list_type::iterator listIterator;
	};

	struct ListData {
		typename map_type::iterator mapIterator;
		TimeType time;
	};

	size_t size() {
		ScopeMutex<Mutex> lock(m_mutex);
		return m_list.size();
	}

public:

//	TimeList(TimeType timeout):m_timeout(timeout) {}

	void add(const K& key, const TimeType& now) {
		ScopeMutex<Mutex> lock(m_mutex);
		MapData mapData;
		pair<typename map_type::iterator, bool> p = m_map.insert(make_pair(key, mapData));
		typename map_type::iterator it = p.first;
		if (p.second == false) {
			m_list.erase(it->second.listIterator);
		}
		m_list.push_back(ListData{it, now + m_timeout});
		it->second.listIterator = --m_list.end();
	}

	void del(const K& key) {
		ScopeMutex<Mutex> lock(m_mutex);
		typename map_type::iterator it = m_map.find(key);
		if (it != m_map.end()) {
			m_list.erase(it->second.listIterator);
			m_map.erase(it);
		}
	}

	void timeout(const TimeType& now) {
		ScopeMutex<Mutex> lock(m_mutex);
		for (typename list_type::iterator it = m_list.begin(); it != m_list.end(); ) {
			if (now >= it->time) {
				if (m_func) m_func(it->mapIterator->first);
				m_map.erase(it->mapIterator);
				it = m_list.erase(it);
			} else {
				break;
			}
		}
	}

	void setTimeout(const TimeType& time) {
		ScopeMutex<Mutex> lock(m_mutex);
		m_timeout = time;
	}

	void setFunction(std::function<void (const K&)> func) {
		ScopeMutex<Mutex> lock(m_mutex);
		m_func = func;
	}

private:
	TimeType m_timeout;
	map_type m_map;
	list_type m_list;
	std::function<void (const K&)> m_func;
	Mutex m_mutex;
};

}

#endif
