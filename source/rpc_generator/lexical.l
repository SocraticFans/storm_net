%option	noyywrap
%option yylineno

%{
#include "parser.h"
#include "syntax.tab.hpp"
#include <sstream>

map<string, int> initMap() {
	map<string, int> m;
	m["service"] = TOK_SERVICE;
	return m;
}

map<string, int> g_map = initMap();

//#define DEBUG_LOG(x) do { cout << "Debug " << x << endl; } while(0);
#define DEBUG_LOG(x) do {} while (0);
#define ERROR_LOG(x) do { ostringstream oss; oss << x << endl; g_parser.error(oss.str());} while(0);

%}

identifier      [[:alpha:]_][[:alnum:]_]*
integer_literal [+-]?((0[0-7]+)|(0[xX][[:xdigit:]]+)|([[:digit:]]+))

%x INCL
%x USINGNS
%x NS

%%

"#include"[ \t]* { BEGIN(INCL); }

<INCL>(\"[^"]*\")|(<[^>]*>) {
	string sText = string(yytext + 1, yytext + strlen(yytext) - 1); 
	DEBUG_LOG("found include: " << sText);
	g_parser.addInclude(sText);
	BEGIN(INITIAL);
}   

<INCL>\n {
	DEBUG_LOG("Invalid include");
}

<INCL><<EOF>> {
	DEBUG_LOG("Invalid include");
}

"using namespace"[ \t]* {BEGIN(USINGNS);}

<USINGNS>{identifier}";" {
	string text = string(yytext, yytext + strlen(yytext) - 1);
	DEBUG_LOG("USINGNS " << text);
	g_parser.addUsingNS(text);
	BEGIN(INITIAL);
}

<USINGNS>\n {
	ERROR_LOG("Invalid USINGNS");
}

<USINGNS><<EOF>> {
	ERROR_LOG("Invalid USINGNS");
}

"package"[ \t]* {BEGIN(NS);}

<NS>{identifier}";" {
	string text = string(yytext, yytext + strlen(yytext) - 1);
	DEBUG_LOG("NS " << text);
	g_parser.setNS(text);
	BEGIN(INITIAL);
}

<NS>\n {
	ERROR_LOG("Invalid NS");
}

<NS><<EOF>> {
	ERROR_LOG("Invalid NS");
}

{identifier} {
	//DEBUG_LOG("lex id "  << yytext);
	map<string, int>::iterator it = g_map.find(yytext);
	if (it != g_map.end()) {
		return it->second;
	}
	yylval.sval = strdup(yytext);
	return TOK_IDENTIFIER;
}

{integer_literal} {
	yylval.ival = atoi(yytext);
	//DEBUG_LOG("lex num "  << yytext);
	return TOK_NUMBER_LITERAL;
}

[[:space:]] { }

. {
	return yytext[0];
}


%%

int main(int argc, char** argv) {
	g_parser.run(argc, argv);

	return 0;
}

