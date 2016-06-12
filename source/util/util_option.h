#ifndef _MFW_UTIL_OPTION_
#define _MFW_UTIL_OPTION_

#include <map>
#include <vector>
#include <string>
using namespace std;

namespace storm
{

class COption
{
public:
	COption():m_stop(false), m_daemon(false) {}

    void parse(int argc, char *argv[]);
	string getConfigFile() { return m_configFile; }
	bool isStop() { return m_stop; }
	bool isDaemon() { return m_daemon; }
	void displayUsage(const string& name);

private:
	bool m_stop;
	bool m_daemon;
	string m_configFile;
};

}

#endif

