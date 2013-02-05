/* -*- c++ -*-
 *
 * file: ast_util.cpp
 * author: KDr2
 *
 */

#include "ast_util.hpp"
#include "cmn_util.hpp"
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


const JSONNode JSONModelGetter::operator()(uint64_t fn){
    JSONNode::const_iterator it = data->find(num2str(fn));
    if(it!=data->end()) {
        return *it;
    }
    return JSONNode();
}

const vector<uint64_t> JSONModelGetter::operator()(const string &rel_key){
    vector<uint64_t> ret;
    JSONNode::const_iterator it = data->find(rel_key);
    if(it==data->end()){//||it->type()!=JSON_ARRAY) {
        return ret;
    }
    JSONNode fns_node = it->as_array();
    if(fns_node.size()>0){
        JSONNode::const_iterator stit = fns_node.begin();
        while(stit!=fns_node.end()){
            uint64_t fn = 0UL;
            if(stit->type()==JSON_NUMBER){
                fn =  stit->as_int();
            }else if(it->type()==JSON_STRING){
                string fs = it->as_string();
                fn = strtoul(fs.c_str(), NULL, 10);
            }
            ret.push_back(fn);
            ++stit;
        }
    }
    return ret;
}
