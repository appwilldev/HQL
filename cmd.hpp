/* -*- c++ -*-
 *
 * file: cmd.hpp
 * author: KDr2
 *
 */

#ifndef _CMD_HPP
#define _CMD_HPP

#include <vector>
#include <string>

#include "ast.hpp"

using std::vector;
using std::string;


class ShellState{
public:

    static void push_hql(HQLNode*);
    static void pop_hql(size_t n=1);
    static HQLNode* top_hql();
    static size_t hql_stack_size(){return hql_stack.size();};
    
private:
    static vector<HQLNode*> hql_stack;
    static vector<string> result_stack;
};


class ShellCommand{
public:
    enum CMD{
        PUSH_HQL, POP_HQL, EXPLAIN_HQL, LIST_HQL, CLEAR_HQL,        
    };

    static bool do_cmd(CMD);
    static bool do_cmd(CMD, HQLNode*);
};


#endif /* _CMD_HPP */


