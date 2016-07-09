#ifndef _PARSER_H_
#define _PARSER_H_

#include <stdint.h>
#include <string>
#include <vector>

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <map>
#include <memory>
#include <sstream>

using namespace std;

struct Function {
	string m_name;
	string m_inputClassName;
	string m_inputParamName;
	string m_outputClassName;
	string m_outputParamName;
	uint32_t m_protoId;
};

struct Service {
	string m_serviceName;
	vector<Function> m_functions;
};

struct Parser {
public:
	void generatorService();
	void generatorServiceProxy();
	Parser(): m_curService(NULL) {}

	void run(int argc, char** argv);
	void debug(const string& msg);
	void error(const string& err);

	void addInclude(const string& file);
	void addUsingNS(const string& ns);
	void setNS(const string& ns);

	void addNewService(const string& service);
	void addNewFunction(const string& funcName,
						const string& inputClassName, const string& inputParamName,
						const string& outputClassName, const string& outputParamName,
						uint32_t protoId);

	void show();
	void generator();
	void generatorHead();
	void generatorSource();

	void printHead();
	void printServiceHead(Service& s);
	void printTail();

	void printServiceSource(Service& s);

	ostringstream& tab(int n);

public:
	string m_fileName;
	vector<string> m_includes;
	vector<string> m_usingNameSpace;
	string m_nameSpace;
	vector<Service> m_services;

	Service* m_curService;
	ostringstream m_oss;
};

extern Parser g_parser;

#endif
