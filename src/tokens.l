/* -*- flex -*-
 *
 * file: tokens.l
 * author: KDr2
 *
 */

%option noyywrap
%option reentrant
%option bison-bridge

%{

#include <stdio.h>

#include <vector>
using std::vector;

#include "ast.hpp"
#include "cmd.hpp"
#include "parser.hpp"

#ifdef DEBUG
#define DEBUGF(x,...) printf("<DEBUG,%s:%d>" x "\n",__FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUGF(x,...)
#endif

#define SAVE_STR(N) yylval->str = new string(yytext+N, yyleng-N)
#define SAVE_COOP(t) yylval->cond_oper = COND_OPER::t
#define SAVE_CMD(t) yylval->cmd = ShellCommand::t
#define TOKEN(t) (yylval->token = t)

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

%}

%x HQL

%%

"<<"                           BEGIN(HQL); return T_HQL_BEGIN;
<HQL>[ \t\n\r]                 ; //ignore
<HQL>">>"                      BEGIN(INITIAL); return T_HQL_END;

<HQL>"(" |
<HQL>")" |
<HQL>"." |
<HQL>"*" |
<HQL>"," |
<HQL>"[" |
<HQL>"]"                       return yytext[0];

<HQL>-?[0-9]+                  SAVE_STR(0); return T_HQL_NUM;
<HQL>-?[0-9]+"."[0-9]+         SAVE_STR(0); return T_HQL_NUM;

<HQL>"#"[0-9]{3,}              SAVE_STR(1); return T_FULLNAME;

<HQL>(?i:auto|@)               return TOKEN(T_HQL_AUTO);
<HQL>(?i:\*auto|\*@)           return TOKEN(T_HQL_TAUTO);
<HQL>(?i:each|"?")             return TOKEN(T_HQL_EACH);
<HQL>(?i:eval)                 return TOKEN(T_HQL_EACH);

<HQL>(?i:all)                  return TOKEN(T_HQL_ALL);
<HQL>(?i:true)                 return TOKEN(T_HQL_TRUE);
<HQL>(?i:false)                return TOKEN(T_HQL_FALSE);

<HQL>(?i:host)                 return TOKEN(T_HQL_HOST);
<HQL>(?i:guest)                return TOKEN(T_HQL_GUEST);
<HQL>(?i:asc)                  return TOKEN(T_HQL_ASC);
<HQL>(?i:desc)                 return TOKEN(T_HQL_DESC);

<HQL>(?i:as)                   return TOKEN(T_HQL_AS);
<HQL>(?i:between)              return TOKEN(T_HQL_BETWEEN);
<HQL>(?i:and)                  return TOKEN(T_HQL_AND);
<HQL>(?i:or)                   return TOKEN(T_HQL_OR);
<HQL>(?i:not)                  return TOKEN(T_HQL_NOT);
<HQL>(?i:select)               return TOKEN(T_HQL_SELECT);
<HQL>(?i:where)                return TOKEN(T_HQL_WHERE);
<HQL>(?i:limit)                return TOKEN(T_HQL_LIMIT);
<HQL>(?i:order)                return TOKEN(T_HQL_ORDER);
<HQL>(?i:by)                   return TOKEN(T_HQL_BY);

<HQL>"="                       SAVE_COOP(EQ); return T_HQL_CO_OP_CMP;
<HQL>">"                       SAVE_COOP(GT); return T_HQL_CO_OP_CMP;
<HQL>"<"                       SAVE_COOP(LT); return T_HQL_CO_OP_CMP;
<HQL>">="                      SAVE_COOP(GE); return T_HQL_CO_OP_CMP;
<HQL>"<="                      SAVE_COOP(LE); return T_HQL_CO_OP_CMP;
<HQL>(?i:time_in)              SAVE_COOP(TIME_IN); return T_HQL_CO_OP_CMP;
<HQL>(?i:"in")                 SAVE_COOP(IN); return T_HQL_CO_OP_IN;
<HQL>(?i:"contains")           SAVE_COOP(CONTAINS); return T_HQL_CO_OP_CTNS;

<HQL>\"(\\\"|[^\"])*[^\\]\"    SAVE_STR(0); return T_HQL_STR;
<HQL>[_a-zA-Z0-9]+             SAVE_STR(0); return T_HQL_ID;
<HQL>.|\n                      DEBUGF("%s", "IGNORE in HQL");

[ \n\r]                        ;


";"                            return yytext[0];

"#"[0-9]{3,}                   SAVE_STR(1); return T_FULLNAME;

(?i:save)                      SAVE_CMD(PUSH_HQL); return T_CMD;
(?i:pop)                       SAVE_CMD(POP_HQL); return T_CMD;
(?i:explain)                   SAVE_CMD(EXPLAIN_HQL); return T_CMD;

<<EOF>>                        yyterminate();


%%

/*<HQL>`"*"?[a-z0-9]+`         printf("id\n"); SAVE_STR(0); return T_HQL_ID;*/
