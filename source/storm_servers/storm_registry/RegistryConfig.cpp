#include "RegistryConfig.h"
#include "util/util_config.h"
#include "util/util_log.h"

namespace Registry {

RegistryConfig g_config;

bool RegistryConfig::init() {
	try {
		CConfig config;
		config.parseFile("Registry.conf");
		dbConfig.host = config.getCfg("/server/DB/host");
		dbConfig.user = config.getCfg("/server/DB/user");
		dbConfig.passwd = config.getCfg("/server/DB/passwd");
		dbConfig.dbname = config.getCfg("/server/DB/dbname");
		dbConfig.port = config.getCfg("/server/DB/port", 3306);
		dbConfig.charset = config.getCfg("/server/DB/charset", "utf8");
	} catch (std::exception& e) {
		LOG_ERROR << e.what();
		return false;
	}
	return true;
}

}
