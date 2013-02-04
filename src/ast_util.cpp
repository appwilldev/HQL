/* -*- c++ -*-
 *
 * file: ast_util.cpp
 * author: KDr2
 *
 */

#include "ast_util.hpp"
#include "ast.hpp"
#include "concrete_ast.hpp"
#include "cmd.hpp"
#include "parser.hpp"
#include "type_config.hpp"


#define YY_EXTRA_TYPE void*

struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
typedef void* yyscan_t;

extern int yylex_init(yyscan_t*);
extern int yylex_init_extra(YY_EXTRA_TYPE, yyscan_t*);
extern YY_BUFFER_STATE yy_scan_string(const char *str, yyscan_t);
extern int yyparse(yyscan_t);
extern int yylex_destroy(yyscan_t);

HQLNode* ASTUtil::parser_hql(const string &hql)
{
    yyscan_t scanner;
    ShellState *shell;
    HQLNode *ret = NULL;
    string s = string("SAVE <<");
    s = s + hql;
    s = s + ">>;";
    shell = new ShellState();
    yylex_init_extra(shell, &scanner);
    yy_scan_string(s.c_str(), scanner);
    yyparse(scanner);
    yylex_destroy(scanner);
    HQLNode *n = shell->top_hql();
    if(n && !n->error()){
        ret = n;
        shell->pop_hql(1, false);
    }
    delete shell;
    return ret;
    //return new ErrorNode("parser error: " + hql);
}

Model ASTUtil::get_model(uint64_t fn, ModelGetter *model_getter)
{
    if(model_getter){
        JSONNode n = model_getter->operator()(fn);
        return Model(n);
    }
    return Model(JSONNode());
}

vector<uint64_t> ASTUtil::get_relations(const string &fn, ModelGetter *model_getter)
{

    if(model_getter){
       return  model_getter->operator()(fn);
    }
    return vector<uint64_t>();
}
