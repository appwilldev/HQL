/* -*- bison -*-
 *
 * file: parser.y
 * author: KDr2
 *
 */

%define api.pure

%{
#include "cmn_util.hpp"
#include "concrete_ast.hpp"
#include "cmd.hpp"

#include <stdio.h>

#include <vector>
#include <iostream>

using std::vector;

#ifdef DEBUG
#undef DEBUG
#define DEBUG(x) std::cout<<"<DEBUG,"<<__FILE__<<":"<<__LINE__<<">"<<x<<std::endl;
#define DEBUGF(x,...) printf("<DEBUG,%s:%d>" x "\n",__FILE__, __LINE__, __VA_ARGS__)
#else
#define DEBUG(x)
#define DEBUGF(x,...)
#endif


#define YYPARSE_PARAM scanner
#define YYLEX_PARAM   scanner

union YYSTYPE;
typedef void* yyscan_t;
extern int yylex(YYSTYPE*, yyscan_t);
void yyerror(const char *s) { printf("ERROR: %s\n", s); }

#define YY_EXTRA_TYPE  void*
YY_EXTRA_TYPE  yyget_extra ( yyscan_t scanner );
void           yyset_extra ( YY_EXTRA_TYPE arbitrary_data , yyscan_t scanner);
%}

%union{
    string *str;
    vector<string> *hql_num_array;
    vector<string> *hql_str_array;
    void *ptr;
    COND_OPER::COND_OPER cond_oper;
    ShellCommand::CMD cmd;
    HQLNode *node;
    HQLOperand *operand;
    int64_t token;
 }


/* tokens */
%token <token> T_HQL_BEGIN T_HQL_END
%token <token> T_HQL_AUTO T_HQL_TAUTO T_HQL_EACH T_HQL_ALL
%token <token> T_HQL_TRUE T_HQL_FALSE
%token <token> T_HQL_HOST T_HQL_GUEST T_HQL_ASC T_HQL_DESC
%token <token> T_HQL_AS T_HQL_BETWEEN
%token <token> T_HQL_AND T_HQL_OR T_HQL_NOT
%token <token> T_HQL_SELECT T_HQL_WHERE
%token <token> T_HQL_LIMIT T_HQL_ORDER T_HQL_BY
%token <str>   T_HQL_ID T_HQL_NUM T_HQL_STR
%token <cond_oper> T_HQL_CO_OP_CMP T_HQL_CO_OP_IN T_HQL_CO_OP_CTNS

%token <str> T_FULLNAME

%token <cmd> T_CMD


%type <hql_num_array> hql_num_array
%type <hql_str_array> hql_str_array
%type <operand> hql_cmp_arg hql_rl_arg hql_contains_arg
%type <node> hql_sl_all hql_sl_ordinary
%type <node> hql_sl_extfk_all hql_sl_extfk
%type <node> hql_sl hql_rl hql_ll_s hql_ll_r hql_ll
%type <node> hql_misc hql_ob
%type <node> hql
%type <str>  hql_ob_arg

%destructor { DEBUG("Release $$"); delete $$;} <node> <str> <hql_num_array> <hql_str_array> <operand>

%left T_HQL_AND T_HQL_OR
%right T_HQL_NOT

%start stmts

%%

hql_num_array : '[' T_HQL_NUM {
                  $$=new vector<string>();
                  $$->push_back(*$2);
                  delete $2;
                }
              | hql_num_array ',' T_HQL_NUM {
                  $$=$1;
                  $1->push_back(*$3);
                  delete $3;
                }
              ;

hql_str_array : '[' T_HQL_STR {
                  $$=new vector<string>();
                  $$->push_back(str_escape_quote(*$2));
                  delete $2;
                }
              | hql_str_array ',' T_HQL_STR {
                  $$=$1;
                  $1->push_back(str_escape_quote(*$3));
                  delete $3;
                }
              ;

hql_cmp_arg: T_HQL_NUM {
               if($1->find(".")==std::string::npos){
                 $$=new HQLOperand(static_cast<HQLOperand::number>(atol($1->c_str())));
               }else{
                 $$=new HQLOperand(static_cast<HQLOperand::number>(atof($1->c_str())));
               }
               delete $1;
             }
           | T_HQL_TRUE {
               $$ = new HQLOperand(true);
             }
           | T_HQL_FALSE {
               $$ = new HQLOperand(false);
             }
           | T_HQL_STR {
               $$=new HQLOperand(str_escape_quote(*$1), HQLOperand::STRING);
               delete $1;
             }
           | T_HQL_EACH {
               $$=new HQLOperand(EACH);
             }
           ;

hql_contains_arg: T_HQL_STR T_HQL_BY T_HQL_STR {
                     vector<string> *v = new vector<string>();
                     v->push_back(str_escape_quote(*$1));
                     delete $1;
                     v->push_back(str_escape_quote(*$3));
                     delete $3;
                     $$=new HQLOperand(v, HQLOperand::CONTAINS_ARG);
                  }
                | T_HQL_EACH T_HQL_BY T_HQL_STR {
                    vector<string> *v = new vector<string>();
                    v->push_back(string("\0", 1));
                    v->push_back(str_escape_quote(*$3));
                    delete $3;
                    $$=new HQLOperand(v, HQLOperand::CONTAINS_ARG);
                  }
                ;

hql_sl : hql_sl_all {$$=$1;}
       | hql_sl_ordinary {$$=$1;}
       | hql_sl_extfk_all {$$=$1;}
       | hql_sl_extfk {$$=$1;}
       | '(' hql_sl ')' {$$=$2;}
       ;


hql_sl_ordinary : T_HQL_SELECT T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CMP hql_cmp_arg {
                    $$=new SLCondNode(*$2, *$4, HQLOperand($5), *$6);
                    DEBUG(*$2);
                    delete $2;
                    delete $4;
                    delete $6;
                  }
                | T_HQL_SELECT '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CMP hql_cmp_arg {
                    $$=new SLCondNode(*$3, *$5, HQLOperand($6), *$7, HQLNode::EXPLICIT);
                    DEBUG($3);
                    delete $3;
                    delete $5;
                    delete $7;
                  }
                | T_HQL_SELECT T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_num_array ']'{
                    $$=new SLCondNode(*$2, *$4, HQLOperand($5), HQLOperand($6, HQLOperand::NUM_ARRAY));
                    DEBUG($2);
                    delete $2;
                    delete $4;
                  }
                | T_HQL_SELECT '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_num_array ']'{
                    $$=new SLCondNode(*$3, *$5, HQLOperand($6), HQLOperand($7, HQLOperand::NUM_ARRAY),
                                      HQLNode::EXPLICIT);
                    DEBUG($3);
                    delete $3;
                    delete $5;
                  }
                | T_HQL_SELECT T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_str_array ']'{
                    $$=new SLCondNode(*$2, *$4, HQLOperand($5), HQLOperand($6, HQLOperand::STR_ARRAY));
                    DEBUG($2);
                    delete $2;
                    delete $4;
                  }
                | T_HQL_SELECT '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_str_array ']'{
                    $$=new SLCondNode(*$3, *$5, HQLOperand($6), HQLOperand($7, HQLOperand::STR_ARRAY),
                                      HQLNode::EXPLICIT);
                    DEBUG($3);
                    delete $3;
                    delete $5;
                  }
                | T_HQL_SELECT T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CTNS hql_contains_arg {
                    $$=new SLCondNode(*$2, *$4, HQLOperand($5), *$6);
                    DEBUG(*$2);
                    delete $2;
                    delete $4;
                    delete $6;
                  }
                | T_HQL_SELECT '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CTNS hql_contains_arg {
                    $$=new SLCondNode(*$3, *$5, HQLOperand($6), *$7, HQLNode::EXPLICIT);
                    DEBUG($3);
                    delete $3;
                    delete $5;
                    delete $7;
                  }
                ;

hql_sl_all : T_HQL_SELECT T_HQL_ID {
               $$ = new SLAllNode(*$2);
               delete $2;
             }
           | T_HQL_SELECT '*' T_HQL_ID {
               $$ = new SLAllNode(*$3, HQLNode::EXPLICIT);
               delete $3;
             }
           ;

hql_sl_extfk : T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CMP hql_cmp_arg {
                 $$ = new SLFKCondNode(*$2, *$4, *$6, *$8, HQLOperand($9), *$10);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $6;
                 delete $8;
                 delete $10;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CMP hql_cmp_arg {
                 $$ = new SLFKCondNode(*$2, *$4, *$7, *$9, HQLOperand($10), *$11, HQLNode::EXPLICIT);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $7;
                 delete $9;
                 delete $11;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_num_array ']'{
                 $$ = new SLFKCondNode(*$2, *$4, *$6, *$8,
                                       HQLOperand($9), HQLOperand($10, HQLOperand::NUM_ARRAY));
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $6;
                 delete $8;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_num_array ']'{
                 $$ = new SLFKCondNode(*$2, *$4, *$7, *$9,
                                       HQLOperand($10), HQLOperand($11, HQLOperand::NUM_ARRAY),
                                       HQLNode::EXPLICIT);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $7;
                 delete $9;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_str_array ']'{
                 $$ = new SLFKCondNode(*$2, *$4, *$6, *$8,
                                       HQLOperand($9), HQLOperand($10, HQLOperand::STR_ARRAY));
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $6;
                 delete $8;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_IN hql_str_array ']'{
                 $$ = new SLFKCondNode(*$2, *$4, *$7, *$9,
                                       HQLOperand($10), HQLOperand($11, HQLOperand::STR_ARRAY),
                                       HQLNode::EXPLICIT);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $7;
                 delete $9;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CTNS hql_contains_arg {
                 $$ = new SLFKCondNode(*$2, *$4, *$6, *$8, HQLOperand($9), *$10);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $6;
                 delete $8;
                 delete $10;
               }
             | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS '*' T_HQL_ID T_HQL_WHERE T_HQL_ID T_HQL_CO_OP_CTNS hql_contains_arg {
                 $$ = new SLFKCondNode(*$2, *$4, *$7, *$9, HQLOperand($10), *$11, HQLNode::EXPLICIT);
                 DEBUG($2);
                 delete $2;
                 delete $4;
                 delete $7;
                 delete $9;
                 delete $11;
               }
             ;

hql_sl_extfk_all : T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS T_HQL_ID {
                     $$ = new SLFKAllNode(*$2, *$4, *$6);
                     delete $2;
                     delete $4;
                     delete $6;
                   }
                 | T_HQL_SELECT T_HQL_ID '.' T_HQL_ID T_HQL_AS '*' T_HQL_ID {
                     $$ = new SLFKAllNode(*$2, *$4, *$7, HQLNode::EXPLICIT);
                     delete $2;
                     delete $4;
                     delete $7;
                   }
                 ;

hql_ll_s : hql_sl {$$=$1;}
         | hql_ll_s T_HQL_AND hql_ll_s {
             $$ = new LogicAndNode(shared_ptr<HQLNode>($1), shared_ptr<HQLNode>($3));
             //delete $1;
             //delete $3;
           }
         | hql_ll_s T_HQL_OR hql_ll_s {
             $$ = new LogicOrNode(shared_ptr<HQLNode>($1), shared_ptr<HQLNode>($3));
             //delete $1;
             //delete $3;
           }
         | T_HQL_NOT hql_ll_s {
             $$ = new LogicNotNode(shared_ptr<HQLNode>($2));
             //delete $2;
           }
         | '(' hql_ll_s ')' {$$=$2;}
         ;


hql_rl_arg : T_HQL_AUTO {
               $$=new HQLOperand(AUTO);
             }
           | T_HQL_TAUTO {
               $$=new HQLOperand(TAUTO);
             }
           | T_HQL_EACH {
               $$=new HQLOperand(EACH);
             }
           | T_FULLNAME {
               $$=new HQLOperand(static_cast<uint64_t>(atol($1->c_str())));
               delete $1;
             }
//         | '(' hql_rl_arg ')' {
//             $$=$2;
//           }
           ;

hql_rl : hql_rl_arg T_HQL_BETWEEN hql_rl_arg T_HQL_AND hql_rl_arg {
           DEBUG("HQL RL");
           $$=new RLNode(*$1, *$3, *$5);
           delete $1;
           delete $3;
           delete $5;
         }
       | hql_ll_s T_HQL_BETWEEN hql_ll_s T_HQL_AND hql_ll_s {
           DEBUG("HQL RL");
           $$=new RLNode(HQLOperand(shared_ptr<HQLNode>($1)),
                         HQLOperand(shared_ptr<HQLNode>($3)),
                         HQLOperand(shared_ptr<HQLNode>($5)));
         }
       | hql_rl_arg T_HQL_BETWEEN hql_ll_s T_HQL_AND hql_ll_s {
           DEBUG("HQL RL");
           $$=new RLNode(*$1,
                         HQLOperand(shared_ptr<HQLNode>($3)),
                         HQLOperand(shared_ptr<HQLNode>($5)));
           delete $1;
         }
       | hql_ll_s T_HQL_BETWEEN hql_rl_arg T_HQL_AND hql_ll_s {
           DEBUG("HQL RL");
           $$=new RLNode(HQLOperand(shared_ptr<HQLNode>($1)),
                         *$3,
                         HQLOperand(shared_ptr<HQLNode>($5)));
           delete $3;
         }
       | hql_ll_s T_HQL_BETWEEN hql_ll_s T_HQL_AND hql_rl_arg {
           DEBUG("HQL RL");
           $$=new RLNode(HQLOperand(shared_ptr<HQLNode>($1)),
                         HQLOperand(shared_ptr<HQLNode>($3)),
                         *$5);
           delete $5;
         }
       | hql_rl_arg T_HQL_BETWEEN hql_rl_arg T_HQL_AND hql_ll_s {
           DEBUG("HQL RL");
           $$=new RLNode(*$1, *$3, HQLOperand(shared_ptr<HQLNode>($5)));
           delete $1;
           delete $3;
         }
       | hql_ll_s T_HQL_BETWEEN hql_rl_arg T_HQL_AND hql_rl_arg {
           DEBUG("HQL RL");
           $$=new RLNode(HQLOperand(shared_ptr<HQLNode>($1)), *$3, *$5);
           delete $3;
           delete $5;
         }
       | hql_rl_arg T_HQL_BETWEEN hql_ll_s T_HQL_AND hql_rl_arg {
           DEBUG("HQL RL");
           $$=new RLNode(*$1, HQLOperand(shared_ptr<HQLNode>($3)), *$5);
           delete $1;
           delete $5;
         }
       | '(' hql_rl ')' {
           DEBUG("HQL RL");
           $$=$2;
         }
       ;

hql_ll_r : hql_rl {$$=$1;}
         | hql_rl T_HQL_AND hql_rl {
             $$ = new LogicAndNode(shared_ptr<HQLNode>($1), shared_ptr<HQLNode>($3));
           }
         ;

hql_ll : hql_ll_s {$$=$1;}
       | hql_ll_r {$$=$1;}
       | '(' hql_ll ')' {$$=$2;}
       ;

hql_ob_arg : T_HQL_ID {
               $$ = $1;
             }
           | T_HQL_HOST '.' T_HQL_ID {
               $$ = new string("host." + *$3);
               delete $3;
             }
           | T_HQL_GUEST '.' T_HQL_ID {
               $$ = $3;
             }
           ;

hql_ob : hql_ll T_HQL_ORDER T_HQL_BY hql_ob_arg {
           $$ = new MiscOrderByNode(shared_ptr<HQLNode>($1),
                                    *$4,
                                    HQLOperand(static_cast<HQLOperand::number>(0)));
           //delete $1;
           delete $4;
         }
       | hql_ll T_HQL_ORDER T_HQL_BY hql_ob_arg T_HQL_ASC {
           $$ = new MiscOrderByNode(shared_ptr<HQLNode>($1),
                                    *$4,
                                    HQLOperand(static_cast<HQLOperand::number>(0)));
           //delete $1;
           delete $4;
         }
       | hql_ll T_HQL_ORDER T_HQL_BY hql_ob_arg T_HQL_DESC {
           $$ = new MiscOrderByNode(shared_ptr<HQLNode>($1),
                                    *$4,
                                    HQLOperand(static_cast<HQLOperand::number>(1)));
           //delete $1;
           delete $4;
       }
       ;

hql_misc : hql_ob { $$=$1; }
         | hql_ob T_HQL_LIMIT T_HQL_NUM {
             $$ = new MiscLimitNode(shared_ptr<HQLNode>($1),
                                      HQLOperand(*$3));
             //delete $1;
             delete $3;
           }
         | hql_ll T_HQL_LIMIT T_HQL_NUM {
             $$ = new MiscLimitNode(shared_ptr<HQLNode>($1),
                                      HQLOperand(*$3));
             //delete $1;
             delete $3;
           }
         ;


hql : hql_ll {
        $$=$1;
      }
    | hql_misc {
        $$=$1;
      }
    | T_HQL_BEGIN hql T_HQL_END {
        $$=$2;
      }
    | error {
        DEBUG("error");
        $$=new ErrorNode("syntax error");
      }
    ;

stmt : T_CMD ';' {
         ShellCommand::do_cmd(yyget_extra(scanner), $1);
       }
     | T_CMD hql ';'{
         DEBUG("HQL CMD");
         ShellCommand::do_cmd(yyget_extra(scanner), $1, $2);
       }
     ;

stmts : stmt
      | stmts stmt
      ;

%%
