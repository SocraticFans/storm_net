#include "RegistryServer.h"
#include "RegistryServiceImp.h"
#include "ServiceManager.h"
#include "RegistryConfig.h"

namespace Registry {

bool RegistryServer::init() {
	addService<RegistryServiceImp>("RegistryService");

	if (!g_config.init()) {
		return false;
	}
	if (!ServiceManager::instance()->init()) {
		return false;
	}
	return true;
}

void RegistryServer::mainLoop() {
	ServiceManager::instance()->update();
}

void RegistryServer::mainLoopDestory() {

}
}

Registry::RegistryServer g_server;

int main(int argc, char** argv) {
	return g_server.run(argc, argv);	
}
