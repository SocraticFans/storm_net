#ifndef _STORM_REGISTRY_H_
#define _STORM_REGISTRY_H_

#include "framework/storm_server.h"
#include "proto/registry.h"

using namespace storm;
using namespace std;

namespace Registry {

class RegistryServer : public StormServer {
public:
	virtual bool init();
	virtual void mainLoop();
	virtual void mainLoopDestory();
};

}


#endif
