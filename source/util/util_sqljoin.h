#ifndef _UTIL_SQL_JOIN_H_
#define _UTIL_SQL_JOIN_H_

#include <iostream>
#include <vector>
#include <string>
#include "util_string.h"

using namespace std;

namespace storm {
class MySqlConn;
class SqlPair {
public:
	friend class MySqlConn;
	enum DB_TYPE {
		DB_INT = 0,	//不需要escape
		DB_STR = 1, //需要escape
	};

	template <typename T>
	SqlPair(const string& key, const T& value) {
		m_key = "`" + key + "`";
		m_value = UtilString::tostr(value);
		m_type = DB_INT;
	}

	SqlPair(const string& key) {
		m_key = "`" + key + "`";
		m_type = DB_INT;
	}

	SqlPair(const string& key, const string& value) {
		m_key = "`" + key + "`";
		m_value = value;
		m_type = DB_STR;
	}

	SqlPair(const string& key, const char* value):SqlPair(key, string(value)) {
	}

	void show() {
		cout << m_key << "  " << m_value << endl;
	}

private:
	DB_TYPE m_type;
	string m_key;
	string m_value;
};

class SqlJoin {
public:
	friend class MySqlConn;
    template <typename T>
    void addPair(const string& key, const T& value) {
		addPair(SQLPair(key, value));
    }

	void addPair(const SqlPair& p) {
		m_pairs.push_back(p);
	}

	SqlJoin& operator << (const SqlPair& p) {
		m_pairs.push_back(p);
		return *this;
	}
private:
	vector<SqlPair> m_pairs;
};

}

#endif
