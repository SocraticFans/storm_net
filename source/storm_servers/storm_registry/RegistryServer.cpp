#include "RegistryServer.h"
#include "RegistryServiceImp.h"

namespace Registry {

bool RegistryServer::init() {
	//addService<>();
	return true;
}

void RegistryServer::mainLoop() {

}

void RegistryServer::mainLoopDestory() {

}
}

Registry::RegistryServer g_server;

int main(int argc, char** argv) {
	return g_server.run(argc, argv);	
}
