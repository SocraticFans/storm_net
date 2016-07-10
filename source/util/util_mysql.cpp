#include "util_mysql.h"

#include <assert.h>
#include <stdio.h>

namespace storm {
MySqlResult::MySqlResult(MYSQL_RES* pRes)
	:m_pRes(pRes)
	,m_iRows(0)
	,m_iFields(0) {

	m_pRes = pRes;
	if (m_pRes == NULL) {
		return;
	}
	m_iRows   = mysql_num_rows(m_pRes);
	m_iFields = mysql_num_fields(m_pRes);

	MYSQL_FIELD* field;
	mysql_field_seek(m_pRes, 0);
	for (uint32_t i = 0; i < m_iFields; ++i) {
		field = mysql_fetch_field(m_pRes);
		m_mField2Index.insert(std::make_pair(field->name, i));
	}
}

MySqlResult::~MySqlResult() {
	if (m_pRes != NULL) {
		mysql_free_result(m_pRes);
	}
}

std::string MySqlResult::get(uint32_t iRow, uint32_t iField) {
	if (iRow >= m_iRows || iField >= m_iFields) {
		return std::string();
	}
	mysql_data_seek(m_pRes, iRow);
	MYSQL_ROW row = mysql_fetch_row(m_pRes);
	if (row == NULL) {
		return std::string();
	}
	if (row[iField] == NULL) {
		return std::string();
	}
	unsigned long* length = mysql_fetch_lengths(m_pRes);
	return std::string(row[iField], length[iField]);
}

std::string MySqlResult::get(uint32_t iRow, const std::string& sFieldName) {
	auto it = m_mField2Index.find(sFieldName);
	//std::map<std::string, uint32_t>::iterator it = m_mField2Index.find(sFieldName);
	if (it == m_mField2Index.end()) {
		return std::string();
	}
	return get(iRow, it->second);
}

MySqlConn::MySqlConn()
	:m_mysql(NULL), m_bConnected(false) {
}

MySqlConn::MySqlConn(const DBConfig& config)
	:m_config(config), m_mysql(NULL), m_bConnected(false) {
}

MySqlConn::MySqlConn(const std::string& host,
				   	 const std::string& user,
				     const std::string& passwd,
				     const std::string& db,
				     const std::string& charset,
				     uint32_t port)
	:m_mysql(NULL), m_bConnected(false) {
	m_config.host = host;
	m_config.user = user;
	m_config.passwd = passwd;
	m_config.dbname = db;
	m_config.charset = charset;
	m_config.port = port;
}

MySqlConn::~MySqlConn() {
	disconnect();
}

void MySqlConn::setConfig(const DBConfig& config) {
	disconnect();
	m_config = config;
}

inline void MySqlConn::disconnect() {
	if (m_mysql != NULL) {
		mysql_close(m_mysql);
		m_mysql = NULL;
		m_bConnected = false;
	}
}

inline void MySqlConn::printError() {
	printf("mysql error: %u (%s) %s\n", mysql_errno(m_mysql), mysql_sqlstate(m_mysql),
		    mysql_error(m_mysql));
}

void MySqlConn::connect() {
	disconnect();
	m_mysql = mysql_init(NULL);
	if (m_mysql == NULL) {
		throw std::runtime_error("mysql_init: " + string(mysql_error(m_mysql)));
	}

	//设置连接、读取、写入超时
	int timeout = 5;
	if (mysql_options(m_mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout)) {
		throw std::runtime_error("mysql_options: " + string(mysql_error(m_mysql)));
	}
	if (mysql_options(m_mysql, MYSQL_OPT_READ_TIMEOUT, &timeout)) {
		throw std::runtime_error("mysql_options: " + string(mysql_error(m_mysql)));
	}
	if (mysql_options(m_mysql, MYSQL_OPT_WRITE_TIMEOUT, &timeout)) {
		throw std::runtime_error("mysql_options: " + string(mysql_error(m_mysql)));
	}

	if (mysql_real_connect(m_mysql, m_config.host.c_str(), m_config.user.c_str(), m_config.passwd.c_str(), m_config.dbname.c_str(),
						   m_config.port, NULL, 0) == NULL) {
		throw std::runtime_error("mysql_real_connect: " + string(mysql_error(m_mysql)));
	}

	char value = 1;
	mysql_options(m_mysql, MYSQL_OPT_RECONNECT, &value);
	if (mysql_set_character_set(m_mysql, m_config.charset.c_str()) != 0) {
		throw std::runtime_error("mysql: " + string(mysql_error(m_mysql)));
	}
	m_bConnected = true;
}

void MySqlConn::execute(const std::string& sql) {
	if (!m_bConnected) {
		connect();
	}

	int ret = mysql_real_query(m_mysql, sql.c_str(), sql.length());
	if (ret != 0) {
		//printError();
		throw std::runtime_error("mysql_real_query: " + string(mysql_error(m_mysql)));
	}
}

MySqlResult::ptr MySqlConn::query(const std::string& sql) {
	execute(sql);
	MYSQL_RES* result = mysql_store_result(m_mysql);
	if (result == NULL  && mysql_field_count(m_mysql) != 0) {
		//printError();
		throw std::runtime_error("mysql_store_result: " + string(mysql_error(m_mysql)) + ", " + sql);
	}
	//printResult(result);
	return MySqlResult::ptr(new MySqlResult(result));
}

string MySqlConn::escape(const string &s) {
	if (!m_bConnected) {
		connect();
	}

	string r;
	r.resize(s.size() * 2 + 1);
	mysql_real_escape_string(m_mysql, &r[0], s.c_str(), s.size());
	r.resize(strlen(r.c_str()));
	return r;
}

uint64_t MySqlConn::getInsertId() {
	if (m_mysql != NULL) {
		return mysql_insert_id(m_mysql);
	}
	return 0;
}

size_t MySqlConn::getAffectedRows() {
	if (m_mysql != NULL) {
		return mysql_affected_rows(m_mysql);
	}
	return 0;
}

MySqlResult::ptr MySqlConn::select(const std::string& tableName, const string& columns, const SqlJoin& cond) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "select " << columns << " from " << tableName;
	if (!cond.m_pairs.empty()) {
		m_oss << " where ";
		for (uint32_t i =0; i < cond.m_pairs.size(); ++i) {
			const SqlPair& p = cond.m_pairs[i];
			if (i == 0) {
				m_oss << p.m_key << "=";
			} else {
				m_oss << " and " << p.m_key << "=";
			}
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << p.m_value;
			} else {
				m_oss << "'" << escape(p.m_value) << "'";
			}
		}
	}
	//cout << m_oss.str() << endl;
	return query(m_oss.str());
}

MySqlResult::ptr MySqlConn::select(const std::string& tableName, const SqlJoin& columns, const SqlJoin& cond) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "select ";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key;
		} else {
			m_oss << "," << p.m_key;
		}
	}
	m_oss << " from " << tableName;
	if (!cond.m_pairs.empty()) {
		m_oss << " where ";
		for (uint32_t i =0; i < cond.m_pairs.size(); ++i) {
			const SqlPair& p = cond.m_pairs[i];
			if (i == 0) {
				m_oss << p.m_key << "=";
			} else {
				m_oss << " and " << p.m_key << "=";
			}
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << p.m_value;
			} else {
				m_oss << "'" << escape(p.m_value) << "'";
			}
		}
	}
	//cout << m_oss.str() << endl;
	return query(m_oss.str());
}

size_t MySqlConn::update(const string& tableName, const SqlJoin& columns, const string& condition) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "update " << tableName << " set ";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key << "=";
		} else {
			m_oss << "," << p.m_key << "=";
		}
		if (p.m_type == SqlPair::DB_INT) {
			m_oss << p.m_value;
		} else {
			m_oss << "'" << escape(p.m_value) << "'";
		}
	}
	m_oss << " " << condition;

	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();
}

size_t MySqlConn::update(const string& tableName, const SqlJoin& columns, const SqlJoin& cond) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "update " << tableName << " set ";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key << "=";
		} else {
			m_oss << "," << p.m_key << "=";
		}
		if (p.m_type == SqlPair::DB_INT) {
			m_oss << p.m_value;
		} else {
			m_oss << "'" << escape(p.m_value) << "'";
		}
	}
	m_oss << " where ";
	for (uint32_t i =0; i < cond.m_pairs.size(); ++i) {
		const SqlPair& p = cond.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key << "=";
		} else {
			m_oss << " and " << p.m_key << "=";
		}
		if (p.m_type == SqlPair::DB_INT) {
			m_oss << p.m_value;
		} else {
			m_oss << "'" << escape(p.m_value) << "'";
		}
	}

	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();

}

size_t MySqlConn::insert(const string& tableName, const SqlJoin& columns) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "insert into " << tableName << "(";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key;
		} else {
			m_oss << "," << p.m_key;
		}
	}
	m_oss << ") values(";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << p.m_value;
			} else {
				m_oss << "'" << escape(p.m_value) << "'";
			}
		} else {
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << "," <<  p.m_value;
			} else {
				m_oss << ",'" << escape(p.m_value) << "'";
			}
		}
	}
	m_oss << ")";

	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();
}

size_t MySqlConn::replace(const string& tableName, const SqlJoin& columns) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "replace into " << tableName << "(";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key;
		} else {
			m_oss << "," << p.m_key;
		}
	}
	m_oss << ") values(";
	for (uint32_t i = 0; i < columns.m_pairs.size(); ++i) {
		const SqlPair& p = columns.m_pairs[i];
		if (i == 0) {
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << p.m_value;
			} else {
				m_oss << "'" << escape(p.m_value) << "'";
			}
		} else {
			if (p.m_type == SqlPair::DB_INT) {
				m_oss << "," <<  p.m_value;
			} else {
				m_oss << ",'" << escape(p.m_value) << "'";
			}
		}
	}
	m_oss << ")";

	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();
}

size_t MySqlConn::deleteFrom(const string& tableName, const string& condition) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "delete from " << tableName << " " << condition;
	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();
}

size_t MySqlConn::deleteFrom(const string& tableName, const SqlJoin& cond) {
	m_oss.clear();
	m_oss.str("");
	m_oss << "delete from " << tableName << " where ";

	for (uint32_t i =0; i < cond.m_pairs.size(); ++i) {
		const SqlPair& p = cond.m_pairs[i];
		if (i == 0) {
			m_oss << p.m_key << "=";
		} else {
			m_oss << " and " << p.m_key << "=";
		}
		if (p.m_type == SqlPair::DB_INT) {
			m_oss << p.m_value;
		} else {
			m_oss << "'" << escape(p.m_value) << "'";
		}
	}
	//cout << m_oss.str() << endl;
	execute(m_oss.str());
	return getAffectedRows();

}

//测试用
void MySqlConn::printResult(MYSQL_RES* result) {
	int iFields = mysql_num_fields(result);

	MYSQL_FIELD* field;
	mysql_field_seek(result, 0);
	for (int i = 0; i < iFields; ++i) {
		field = mysql_fetch_field(result);
		if (i > 0) {
			printf("\t");
		}
		printf("%s", field->name);
	}
	printf("\n");


	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result)) != NULL) {
		for (int i = 0; i < iFields; ++i) {
			if (i > 0) {
				printf("\t");
			}
			printf("%s", row[i] != NULL ? row[i] : "NULL");
		}
		printf("\n");
	}
}

}

