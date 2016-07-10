#ifndef _STORM_REGISTRY_CONFIG_H_
#define _STORM_REGISTRY_CONFIG_H_

#include <map>
#include <string>
#include <vector>

#include "util/util_thread.h"
#include "util/util_singleton.h"
#include "util/util_mysql.h"


using namespace std;
using namespace storm;

namespace Registry {

struct RegistryConfig {
	bool init();

	DBConfig dbConfig;
};

extern RegistryConfig g_config;

}
#endif
