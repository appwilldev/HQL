/* -*- c++ -*-
 *
 * file: cmd.cpp
 * author: KDr2
 *
 */

#include "cmd.hpp"

vector<HQLNode*> ShellState::hql_stack;
vector<string> ShellState::result_stack;


void ShellState::push_hql(HQLNode* n)
{
    HQLNode *n1 = n->reduce();
    delete n;
    ShellState::hql_stack.push_back(n1);
}


void ShellState::pop_hql(size_t n)
{
    if(n==0) return;
    size_t ssize = hql_stack.size();
    if(ssize<1) return;
    if(n >= ssize){
        vector<HQLNode*>::iterator it = hql_stack.begin();
        while(it!=hql_stack.end()) delete *it++;
        hql_stack.clear();
    }
    ssize = ssize - n;
    vector<HQLNode*>::iterator it = hql_stack.begin() + ssize;
    vector<HQLNode*>::iterator te = it;
    while(it!=hql_stack.end()) delete *it++;
    hql_stack.erase(te, hql_stack.end());
}

HQLNode* ShellState::top_hql(){
    size_t ssize = hql_stack.size();
    if(ssize>0) return *(--hql_stack.end());
    return NULL;
}


bool ShellCommand::do_cmd(ShellCommand::CMD cmd){
    switch(cmd){
    case POP_HQL:
        ShellState::pop_hql();
        return true;
        break;
    default:
        break;
    }
    return false;
}

bool ShellCommand::do_cmd(ShellCommand::CMD cmd, HQLNode* n){
    switch(cmd){
    case PUSH_HQL:
        ShellState::push_hql(n);
        return true;
        break;
    default:
        break;
    }
    return false;
}


