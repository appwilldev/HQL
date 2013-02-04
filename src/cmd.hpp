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

    void push_hql(HQLNode*);
    void pop_hql(size_t n=1, bool delloc=false);
    HQLNode* top_hql();
    size_t hql_stack_size(){return hql_stack.size();};

private:
    vector<HQLNode*> hql_stack;
    vector<string> result_stack;
};


class ShellCommand{
public:
    enum CMD{
        PUSH_HQL, POP_HQL, EXPLAIN_HQL, LIST_HQL, CLEAR_HQL,
    };

    static bool do_cmd(void*, CMD);
    static bool do_cmd(void*, CMD, HQLNode*);
};


#endif /* _CMD_HPP */
