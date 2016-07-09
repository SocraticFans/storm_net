%{
#include "parser.h"
#include "lex.yy.hpp"

#define SYNTAX_ERROR(msg) do { g_parser.error((msg)); YYABORT; } while(0)
#define yyerror SYNTAX_ERROR

%}

%union
{
	int ival;
	char* sval;
}

%token <sval>TOK_IDENTIFIER
%token <sval>TOK_SERVICE
%token <ival>TOK_NUMBER_LITERAL

%%

definitions: 
	| definitions rpc_def
	;

rpc_def:
	TOK_SERVICE TOK_IDENTIFIER
	{
	 	//cout << "rpc " <<  $2 << endl;
	 	g_parser.addNewService($2);
	}
	'{' func_list '}' ';'
	{
	 	g_parser.m_curService = NULL;
	}
	;

func_list:
	| func_list func_def
	;

func_def:
	TOK_IDENTIFIER '(' TOK_IDENTIFIER TOK_IDENTIFIER ',' TOK_IDENTIFIER TOK_IDENTIFIER ')' '=' TOK_NUMBER_LITERAL ';'
	{
		//cout << "Function " << $1 << " " << $3 << " " << $4  << " " << $6 << " " << $7 << " " << $10 << endl;
		g_parser.addNewFunction($1, $3, $4, $6, $7, $10);
	}
	;

%%
