#include "parser.h"
#include "lex.yy.hpp"
#include "syntax.tab.hpp"

#include "util/util_file.h"
#include "util/util_string.h"

extern int yyparse ();
Parser g_parser;

using namespace storm;
void help() {
	cerr << "Usage generator input_file [output_dir]\n";
}

void Parser::run(int argc, char** argv) {
	if (argc < 2) {
		help();
		exit(1);
	}
	string fileName = argv[1];
	yyin = fopen(fileName.c_str(), "r");
	if (yyin == NULL) {
		cout << "can't open file " << fileName << endl;
		exit(1);
	}
	yypush_buffer_state(yy_create_buffer(yyin, /*YY_BUF_SIZE*/ 16 * 1024));

	int ret = yyparse();
	if (ret != 0) {
		cout << "error: " << ret << endl;
		exit(1);
	}

	m_fileName = UtilFile::replaceFileExt(UtilFile::getFileBasename(fileName), "", true);

	//show();
	generator();
	cout << "ok!" << endl;
}

void Parser::error(const string& err) {
	cerr << "Error: line: " << yylineno << " : " << err << endl;
	exit(1);
}

void Parser::debug(const string& msg) {
	cout << "Debug " << msg << endl;
}

void Parser::addInclude(const string& file) {
	m_includes.push_back(file);
}

void Parser::addUsingNS(const string& ns) {
	m_usingNameSpace.push_back(ns);
}

void Parser::setNS(const string& ns) {
	m_nameSpace = ns;
}

void Parser::addNewService(const string& service) {
	Service s;
	s.m_serviceName = service;
	m_services.push_back(s);
	m_curService = &m_services[m_services.size() -1];
}

void Parser::addNewFunction(const string& funcName,
						const string& inputClassName, const string& inputParamName,
						const string& outputClassName, const string& outputParamName,
						uint32_t protoId) {
	if (m_curService == NULL) {
		error("curService null");
	}
	Function f;
	f.m_name = funcName;
	f.m_inputClassName = inputClassName;
	f.m_inputParamName = inputParamName;
	f.m_outputClassName = outputClassName;
	f.m_outputParamName = outputParamName;
	f.m_protoId = protoId;
	m_curService->m_functions.push_back(f);
}

void Parser::show() {
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		Service& s = m_services[i];
		cout << "Service: " << s.m_serviceName << endl;
		for (uint32_t j = 0; j < s.m_functions.size(); ++j) {
			Function& f = s.m_functions[j];
			cout << "\t" << f.m_name << endl;
			cout << "\t\t" << f.m_inputClassName
				<< "\t" << f.m_inputParamName
				<< "\t" << f.m_outputClassName
				<< "\t" << f.m_outputParamName
				<< "\t" << f.m_protoId << endl;
		}
	}
}

ostringstream& Parser::tab(int n) {
	for (int i = 0; i < n; ++i) {
		m_oss << "\t";
	}
	return m_oss;
}

void Parser::generator() {
	generatorHead();
	generatorSource();
}

void Parser::generatorHead() {
	m_oss.str("");
	m_oss << "/*代码生成器自动生成，请不要手动修改!*/" << endl;
	printHead();
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		printServiceHead(m_services[i]);
	}
	printTail();
	
	UtilFile::saveToFile(m_fileName + ".h", m_oss.str());
}

void Parser::generatorSource() {
	m_oss.str("");
	m_oss << "/*代码生成器自动生成，请不要手动修改!*/" << endl;
	m_oss << "#include \"" << m_fileName << ".h\"" << endl;
	m_oss << endl << endl;
	m_oss << "using namespace storm;" << endl;
	m_oss << endl;
	if (!m_nameSpace.empty()) {
		m_oss << "namespace " << m_nameSpace << " {" << endl;
	}
	for (uint32_t i = 0; i < m_services.size(); ++i) {
		printServiceSource(m_services[i]);
	}
	if (!m_nameSpace.empty()) {
		m_oss << endl;
		m_oss << "}" << endl;
	}

	UtilFile::saveToFile(m_fileName + ".cpp", m_oss.str());
}

void Parser::printHead() {
	m_oss << "#ifndef _STORM_RPC_" << UtilString::toupper(m_fileName) << "_H_" <<  endl;
	m_oss << "#define _STORM_RPC_" << UtilString::toupper(m_fileName) << "_H_" <<  endl;
	m_oss << endl;
	m_oss << "#include \"framework/storm_service.h\"" << endl;
	m_oss << "#include \"framework/storm_service_proxy.h\"" << endl;
	m_oss << endl;

	for (uint32_t i = 0; i < m_includes.size(); ++i) {
		string s = m_includes[i];
		m_oss << "#include \"" << UtilFile::replaceFileExt(s, ".pb.h", true) << "\"" << endl;
	}

	//m_oss << "using namespace storm;" << endl;
	for (uint32_t i = 0; i < m_usingNameSpace.size(); ++i) {
		string s = m_usingNameSpace[i];
		m_oss << "using namespace " << s << ";" << endl;
	}

	m_oss << endl;

	if (!m_nameSpace.empty()) {
		m_oss << "namespace " << m_nameSpace << " {" << endl;
	}

}

void Parser::printTail() {
	if (!m_nameSpace.empty()) {
		m_oss << "}" << endl;
	}
	m_oss << endl;
	m_oss << "#endif" << endl;
}

void Parser::printServiceHead(Service& s) {
	//Service
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << " : public storm::StormService {" << endl;
	m_oss << "public:" << endl;
	tab(1) << s.m_serviceName << "(storm::SocketLoop* loop, storm::StormListener* listener)" << endl;
	tab(2) << ":storm::StormService(loop, listener) {" << endl;
	tab(1) << "}" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "(){}" << endl;

	m_oss << endl;
	m_oss << "\t" << "virtual int32_t onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp);" << endl;

	m_oss << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		m_oss << "\t" << "virtual int32_t " << f.m_name << "(const storm::Connection& conn, const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ", " << f.m_outputClassName
			<< "& " << f.m_outputParamName << ") {return 0;}" << endl;
	}
	m_oss << "};" << endl;

	//CallBack
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << "ProxyCallBack : public storm::ServiceProxyCallBack {" << endl;
	m_oss << "public:" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "ProxyCallBack(){}" << endl;
	m_oss << endl;
	m_oss << "\t" << "virtual void dispatch(storm::RequestMessage* req);" << endl;
	m_oss << endl;

	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		m_oss << "\t" << "virtual void callback_" << f.m_name << "(int32_t ret, const "
			<< f.m_outputClassName << "& " << f.m_outputParamName << ") {" << endl;
		tab(2) << "throw std::runtime_error(\"no implement callback_" << f.m_name << "\");" << endl;
		tab(1) << "};" << endl;
	}
	m_oss << "};" << endl;

	//Proxy
	m_oss << endl;
	m_oss << "class " << s.m_serviceName << "Proxy : public storm::ServiceProxy {" << endl;
	m_oss << "public:" << endl;
	m_oss << "\t" << "virtual ~" << s.m_serviceName << "Proxy(){}" << endl;
	m_oss << endl;

	tab(1) << s.m_serviceName << "Proxy* hash(uint64_t code) {" << endl;
	tab(2) << "storm::ServiceProxy::hash(code);" << endl;
	tab(2) << "return this;" << endl;
	tab(1) << "}" << endl;

	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		m_oss << endl;
		Function& f = s.m_functions[i];
		m_oss << "\t" << "int32_t " << f.m_name << "(const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ", " << f.m_outputClassName
			<< "& " << f.m_outputParamName << ");" << endl;
		m_oss << "\t" << "void " << "async_" << f.m_name << "(" << s.m_serviceName << "ProxyCallBack* cb, const "
			<< f.m_inputClassName << "& " << f.m_inputParamName << ", bool broadcast = false);" << endl;
	}
	m_oss << "};" << endl;
} 

void Parser::printServiceSource(Service& s) {
	//Service
	m_oss << "int32_t " << s.m_serviceName << "::onRpcRequest(const storm::Connection& conn, const storm::RpcRequest& req, storm::RpcResponse& resp) {" << endl;

	tab(1) << "int32_t ret = 0;" << endl;
	m_oss << "\t" << "switch (req.proto_id()) {" << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		tab(2) << "case " << f.m_protoId << ":" << endl;
		tab(2) << "{" << endl;
		tab(3) << f.m_inputClassName << " __request;" << endl;
		tab(3) << f.m_outputClassName << " __response;" << endl;
		tab(3) << "if (!__request.ParseFromString(req.request())) {" << endl;
		tab(4) << "STORM_ERROR << \"error\";" << endl;
		tab(4) << "return ResponseStatus_CoderError;" << endl;
		tab(3) << "}" << endl;
		tab(3) << "ret = " << f.m_name << "(conn, __request, __response);" << endl;
		tab(3) << "if (req.invoke_type() != InvokeType_OneWay) {" << endl;
		tab(4) << "if (!__response.SerializeToString(resp.mutable_response())) {" << endl;
		tab(5) << "STORM_ERROR << \"error\"; " << endl;
		tab(5) << "return ResponseStatus_CoderError;" << endl;
		tab(4) << "}" << endl;
		tab(3) << "}" << endl;
		tab(3) << "break;" << endl;
		tab(2) << "}" << endl;
	}
	tab(2) << "default:" << endl;
	tab(3) << "return ResponseStatus_NoProtoId;" << endl;
	tab(1) << "}" << endl;

	m_oss << endl;
	tab(1) << "return ret;" << endl;
	m_oss << "}" << endl;

	//ServiceProxyCallBack
	m_oss << endl;
	m_oss << "void " << s.m_serviceName << "ProxyCallBack::" << "dispatch(storm::RequestMessage* req) {" << endl;
	tab(1) << "uint32_t protoId = req->req.proto_id();" << endl;
	tab(1) << "switch (protoId) {" << endl;
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		Function& f = s.m_functions[i];
		tab(2) << "case " << f.m_protoId << ":" << endl;
		tab(2) << "{" << endl;
		tab(3) << f.m_outputClassName << " __response;" << endl;
		tab(3) << "int32_t ret = decodeResponse(req, __response);" << endl;
		tab(3) << "callback_" << f.m_name << "(ret, __response);" << endl;
		tab(3) << "break;" << endl;
		tab(2) << "}" << endl;
	}
	tab(2) << "default:" << endl;
	tab(2) << "{" << endl;
	tab(3) << "STORM_ERROR << \"unkown protoId \" << protoId;" << endl;
	tab(2) << "}" << endl;
	tab(1) << "}" << endl;
	tab(1) << "storm::ServiceProxy::delRequest(req);" << endl;
	m_oss << "}" << endl;

	//ServiceProxy
	for (uint32_t i = 0; i < s.m_functions.size(); ++i) {
		m_oss << endl;
		Function& f = s.m_functions[i];

		//同步
		m_oss <<  "int32_t " << s.m_serviceName << "Proxy::" << f.m_name << "(const "
			<< f.m_inputClassName << "& request" << ", " << f.m_outputClassName
			<< "& response) {" << endl;
		tab(1) << "storm::RequestMessage* message = newRequest(InvokeType_Sync);" << endl;
		tab(1) << "message->req.set_proto_id(" << f.m_protoId << ");" << endl;
		tab(1) << "request.SerializeToString(message->req.mutable_request());" << endl;
		m_oss << endl;
		tab(1) << "doInvoke(message);" << endl;
		tab(1) << "int32_t ret = decodeResponse(message, response);" << endl;
		m_oss << endl;
		tab(1) << "delRequest(message);" << endl;
		tab(1) << "return ret;" << endl;
		m_oss << "}" << endl;

		m_oss << endl;

		//异步
		m_oss << "void " << s.m_serviceName << "Proxy::" << "async_" << f.m_name << "(" << s.m_serviceName << "ProxyCallBack* cb, const " << f.m_inputClassName << "& request, bool broadcast) {" << endl;
		tab(1) << "storm::RequestMessage* message = newRequest(InvokeType_Async, cb, broadcast);" << endl;
		tab(1) << "uint32_t invokeType = message->invokeType;" << endl;
		tab(1) << "message->req.set_proto_id(" << f.m_protoId << ");" << endl;
		tab(1) << "request.SerializeToString(message->req.mutable_request());" << endl;
		m_oss << endl;
		tab(1) << "doInvoke(message);" << endl;
		m_oss << endl;
		tab(1) << "if (invokeType == InvokeType_OneWay) {" << endl;
		tab(2) << "delRequest(message);" << endl;
		tab(1) << "}" << endl;
		m_oss << "}" << endl;
	}
}





