#ifndef _UTIL_MYSQL_H_
#define _UTIL_MYSQL_H_

#include <memory>
#include <map>
#include <stdint.h>
#include <string>
#include <sstream>

#include <my_global.h>
#include <mysql.h>

#include "util_misc.h"
#include "util_string.h"
#include "util_sqljoin.h"

namespace storm {

struct DBConfig {
	std::string host;
	std::string user;
	std::string passwd;
	std::string dbname;
	std::string charset;
	uint32_t port;

	// TODO
	// parseFromString();

	DBConfig():charset("utf8"), port(3306){}
};

class MySqlResult : public noncopyable {
public:
	typedef std::shared_ptr<MySqlResult> ptr;

	MySqlResult(MYSQL_RES* pRes);
	~MySqlResult();

	uint32_t size() {
		return m_iRows;
	}
	std::string get(uint32_t iRow, uint32_t iField);
	std::string get(uint32_t iRow, const std::string& sFieldName);
	template<typename T>
	T get(uint32_t iRow, uint32_t iField) {
		return UtilString::strto<T>(get(iRow, iField));
	}
	template<typename T>
	T get(uint32_t iRow, const std::string& sFieldName) {
		return UtilString::strto<T>(get(iRow, sFieldName));
	}

private:
	MYSQL_RES* m_pRes;
	uint32_t m_iRows;
	uint32_t m_iFields;
	std::map<std::string, uint32_t> m_mField2Index;
};

class MySqlConn : public noncopyable {
public:
	MySqlConn();
	MySqlConn(const DBConfig& config);
	MySqlConn(const std::string& host,
			  const std::string& user,
		      const std::string& passwd,
		      const std::string& db,
		      const std::string& charset = std::string("utf8"),
		      uint32_t		     port = 3306);

	~MySqlConn();

	void setConfig(const DBConfig& config);
	void execute(const std::string& sql);
	MySqlResult::ptr query(const std::string& sql);

	string escape(const string &s);
	uint64_t getInsertId();
	size_t getAffectedRows();

	MySqlResult::ptr select(const std::string& tableName, const string& columns, const SqlJoin& cond = SqlJoin());
	MySqlResult::ptr select(const std::string& tableName, const SqlJoin& columns, const SqlJoin& cond = SqlJoin());
	size_t update(const string& tableName, const SqlJoin& columns, const string& condition);
	size_t update(const string& tableName, const SqlJoin& columns, const SqlJoin& cond);
	size_t insert(const string& tableName, const SqlJoin& columns);
	size_t replace(const string& tableName, const SqlJoin& columns);
	size_t deleteFrom(const string& tableName, const string& condition);
	size_t deleteFrom(const string& tableName, const SqlJoin& cond);



private:
	void connect();
	void disconnect();
	void printError();
	void printResult(MYSQL_RES* result);

private:
	DBConfig 	m_config;
	MYSQL* 		m_mysql;
	ostringstream m_oss;
	bool 		m_bConnected;
};
}
#endif
