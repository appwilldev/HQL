/* -*- c++ -*-
 *
 * file: ast_misc.cpp
 * author: KDr2
 *
 */

#include "cmn_util.hpp"
#include "concrete_ast.hpp"
#include "type_config.hpp"

#include <iostream>
#include <sstream>


/* MiscOrderByNode */

MiscOrderByNode::MiscOrderByNode(shared_ptr<HQLNode> o,
                                 const string &attr,
                                 const HQLOperand &asc):
    HQLNode(MISC, ORDER_BY, o->get_target())
{
    etype = o->get_etype();
    push_operand(HQLOperand(o));
    push_operand(HQLOperand(attr));
    push_operand(asc);
}


MiscOrderByNode::MiscOrderByNode(const MiscOrderByNode &n):
    HQLNode(n)
{
}

const MiscOrderByNode& MiscOrderByNode::operator=(const MiscOrderByNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

MiscOrderByNode::~MiscOrderByNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* MiscOrderByNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }

    HQLNode *n = operands[0].as_node()->reduce();
    return new MiscOrderByNode(shared_ptr<HQLNode>(n),
                               *operands[1].as_str(),
                               operands[2]);
}

HQLNode* MiscOrderByNode::copy()
{
    return new MiscOrderByNode(*this);
}


const string MiscOrderByNode::to_hql() const
{
    return operands[0].as_node()->to_hql() +
        string(" ORDER BY ") + *operands[1].as_str() +
        (operands[2].as_num()==0 ? string(" ASC") : string(" DESC"));
}

const string MiscOrderByNode::cache_key(bool do_result_reduce) const
{
    return string("O[") + *operands[1].as_str() +
        (operands[2].as_num()==0 ? string(",A,") : string(",D,")) +
        operands[0].as_node()->cache_key(do_result_reduce) + "]";
}

const set<string> MiscOrderByNode::get_ctypes() const{
    return operands[0].as_node()->get_ctypes();
}

const set<string> MiscOrderByNode::get_rtypes() const{
    return operands[0].as_node()->get_rtypes();
}

ExtraMatchDataInfo MiscOrderByNode::get_xmdinfo() const{
    return operands[0].as_node()->get_xmdinfo();
}

const map<uint64_t, set<string> > MiscOrderByNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, tmp;
    if(m.attr<bool>("deleted")) return ret;
    tmp = operands[0].as_node()->match(m, getter);
    map<uint64_t, set<string> >::iterator it = tmp.begin();
    for(; it!=tmp.end(); ++it){
        set<string>::iterator kt = it->second.begin();
        for(; kt!=it->second.end(); ++kt){
            MatchedNode *n0 = new MatchedNode(*kt);
            MiscOrderByNode n(shared_ptr<HQLNode>(n0),
                              *operands[1].as_str(),
                              operands[2]);
            ret[it->first].insert(n.cache_key());
        }
    }
    return ret;
}

bool MiscOrderByNode::validate() const
{
    if(!operands[0].as_node()->validate()){
        error_info = "bad sub statement: " + operands[0].as_node()->to_hql();
        return false;
    }
    etype = operands[0].as_node()->get_etype();
    return true;
}

bool MiscOrderByNode::has_semantic_each() const
{
    return operands[0].as_node()->has_semantic_each();
}

pair<string, uint64_t> MiscOrderByNode::time_in() const
{
    return operands[0].as_node()->time_in();
}

vector<string> MiscOrderByNode::has_semantic_fk() const
{
    return operands[0].as_node()->has_semantic_fk();
}

/* MiscLimitNode */

MiscLimitNode::MiscLimitNode(shared_ptr<HQLNode> o,
                             const HQLOperand &num):
    HQLNode(MISC, LIMIT, o->get_target())
{
    etype = o->get_etype();
    push_operand(HQLOperand(o));
    push_operand(num);
}


MiscLimitNode::MiscLimitNode(const MiscLimitNode &n):
    HQLNode(n)
{
}

const MiscLimitNode& MiscLimitNode::operator=(const MiscLimitNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

MiscLimitNode::~MiscLimitNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* MiscLimitNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }

    HQLNode *n = operands[0].as_node()->reduce();
    return new MiscLimitNode(shared_ptr<HQLNode>(n), operands[1]);
}

HQLNode* MiscLimitNode::copy()
{
    return new MiscLimitNode(*this);
}

const string MiscLimitNode::to_hql() const
{
    return operands[0].as_node()->to_hql() +
        string(" LIMIT ") + *operands[1].as_str();
}

const string MiscLimitNode::cache_key(bool do_result_reduce) const
{
    // Limit as Postfix
    return operands[0].as_node()->cache_key(do_result_reduce) + "L" + *operands[1].as_str();
}

const set<string> MiscLimitNode::get_ctypes() const{
    return operands[0].as_node()->get_ctypes();
}

const set<string> MiscLimitNode::get_rtypes() const{
    return operands[0].as_node()->get_rtypes();
}

ExtraMatchDataInfo MiscLimitNode::get_xmdinfo() const{
    return operands[0].as_node()->get_xmdinfo();
}

const map<uint64_t, set<string> > MiscLimitNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, tmp;
    if(m.attr<bool>("deleted")) return ret;
    tmp = operands[0].as_node()->match(m, getter);
    map<uint64_t, set<string> >::iterator it = tmp.begin();
    for(; it!=tmp.end(); ++it){
        set<string>::iterator kt = it->second.begin();
        for(; kt!=it->second.end(); ++kt){
            MatchedNode *n0 = new MatchedNode(*kt);
            MiscLimitNode n(shared_ptr<HQLNode>(n0),
                              operands[1]);
            ret[it->first].insert(n.cache_key());
        }
    }
    return ret;
}

bool MiscLimitNode::validate() const
{
    if(!operands[0].as_node()->validate()){
        error_info = "bad sub statement: " + operands[0].as_node()->to_hql();
        return false;
    }
    etype = operands[0].as_node()->get_etype();
    return true;
}

bool MiscLimitNode::has_semantic_each() const
{
    return operands[0].as_node()->has_semantic_each();
}

pair<string, uint64_t> MiscLimitNode::time_in() const
{
    return operands[0].as_node()->time_in();
}

vector<string> MiscLimitNode::has_semantic_fk() const
{
    return operands[0].as_node()->has_semantic_fk();
}
