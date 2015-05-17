%{
/**
 *		Tempesta Language
 *
 * TL parser and grammar definition.
 *
 * Copyright (C) 2015 Tempesta Technologies, Inc.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 59
 * Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "ast.h"
#include "exception.h"
#include "sym_tbl.h"

#define YYDEBUG 1
%}
 
%require "3.0.0"
%skeleton "lalr1.cc"
%output "parser.cc"
%defines "parser.h"
%locations
%define api.value.type variant
%define api.namespace { tl }
%define parser_class_name { BisonParser }
%parse-param { tl::FlexScanner &scanner }
%parse-param { tl::AST &ast }
%parse-param { tl::SymTbl &st }
%lex-param { tl::FlexScanner &scanner }

%code requires {
	namespace tl {
		class FlexScanner;
	}
}

%code {
	static int yylex(tl::BisonParser::semantic_type * yylval,
			 tl::BisonParser::location_type *loc,
			 tl::FlexScanner &scanner);
}

%left "||"
%left "&&"
%nonassoc "==" "!="
%nonassoc '>' '<' ">=" "<="
%nonassoc "=~" "!~"
%left '.' '(' ')'
 
%token <std::string>	IDENT
%token <std::string>	IPV4
%token <long>		LONGINT
%token <std::string>	STR
%token <std::string>	RE
%token			EQ
%token			NEQ
%token			REEQ
%token			RENEQ
%token			GE
%token			LE
%token			AND
%token			OR
%token			IF

%type <tl::Expr *>	stmt expr ident
%type <tl::Expr::FArgs>	args

%start program

%%
 
program:
	stmt
		{ ast.push_expr($1); }
	| program stmt
		{ ast.push_expr($2); }
	;

stmt:
	expr ';'
		{ $$ = $1; }
	| IF '(' expr ')' stmt
		{ $$ = create_op(TL_IF, $3, $5); }
	;

expr:
	ident '(' ')'
		{ $$ = create_func_noargs($1); }
	| ident '(' args ')'
		{ $$ = create_func($1, $3); }
	| '(' expr ')'
		{ $$ = $2; }
	| expr AND expr
		{ $$ = create_op(TL_AND, $1, $3); }
	| expr OR expr
		{ $$ = create_op(TL_OR, $1, $3); }
	| expr EQ expr
		{ $$ = create_op(TL_EQ, $1, $3); }
	| expr NEQ expr
		{ $$ = create_op(TL_NEQ, $1, $3); }
	| expr REEQ expr
		{ $$ = create_op(TL_REEQ, $1, $3); }
	| expr RENEQ expr
		{ $$ = create_op(TL_RENEQ, $1, $3); }
	| expr '>' expr
		{ $$ = create_op(TL_GT, $1, $3); }
	| expr GE expr
		{ $$ = create_op(TL_GE, $1, $3); }
	| expr '<' expr
		{ $$ = create_op(TL_LT, $1, $3); }
	| expr LE expr
		{ $$ = create_op(TL_LE, $1, $3); }
	| ident
		{ $$ = $1; }
	| LONGINT
		{ $$ = create_number($1); }
	| IPV4
		{ $$ = create_ipv4($1); }
	| STR
		{ $$ = create_str(TL_STR, $1); }
	| RE
		{ $$ = create_str(TL_REGEX, $1); }
	;

ident:
	IDENT
		{ $$ = create_identifier($1, st.lookup($1)); }
	| ident '.' IDENT
		{ $$ = create_deref($1, $3); }
	;

args:
	expr
		{
			$$ = tl::Expr::FArgs();
			$$.push_back($1);
		}
	| args ',' expr
		{
			tl::Expr::FArgs &args = $1;
			args.push_back($3);
			$$ = args;
		}
	;
	
%%

extern int yylineno;

void
tl::BisonParser::error(const tl::BisonParser::location_type &loc,
		       const std::string &msg)
{
	throw TfwExcept("%d: %u:%u %u:%u %s", yylineno,
			loc.begin.line, loc.begin.column,
			loc.end.line, loc.end.column,
			msg.c_str());
}

#include "scanner.h"

static int
yylex(tl::BisonParser::semantic_type * yylval,
      tl::BisonParser::location_type *loc,
      tl::FlexScanner &scanner)
{
	return scanner.yylex(yylval);
}
