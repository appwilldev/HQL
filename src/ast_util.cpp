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


struct yy_buffer_state;
typedef yy_buffer_state *YY_BUFFER_STATE;
extern YY_BUFFER_STATE yy_scan_string(const char *str);
extern int yyparse();
extern int yylex_destroy(void);

HQLNode* ASTUtil::parser_hql(const string &hql, bool autopop)
{
    HQLNode *ret = NULL;
    string s = string("SAVE <<");
    s = s + hql;
    s = s + ">>;";
    yy_scan_string(s.c_str());
    yyparse();
    yylex_destroy();
    HQLNode *n = ShellState::top_hql();
    if(n){
        if(!n->error()){
            ret = autopop ? n->copy() : n;
        }
        if(autopop){
            ShellState::pop_hql();
        }
    }
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
