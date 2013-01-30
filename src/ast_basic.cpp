/* -*- c++ -*-
 *
 * file: ast_basic.cpp
 * author: KDr2
 *
 */


#include "ast_util.hpp"
#include "cmn_util.hpp"
#include "type_config.hpp"
#include "concrete_ast.hpp"

#include <iterator>
#include <algorithm>

/* MatchedNode */

MatchedNode::MatchedNode(const string &ck, HQLNode::TARGET_TYPE tt):
    HQLNode(ERR, NORMAL, tt)
{
    etype = "basic-node(MatchedNode)";
    push_operand(HQLOperand(ck));
}


MatchedNode::MatchedNode(const MatchedNode &n):
    HQLNode(n)
{
}

const MatchedNode& MatchedNode::operator=(const MatchedNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

MatchedNode::~MatchedNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: (MatchedNode)"  << this->to_hql() << std::endl;
#endif
}

HQLNode* MatchedNode::reduce()
{
    return this->copy();
}

HQLNode* MatchedNode::copy()
{
    return new MatchedNode(*this);
}


const string MatchedNode::to_hql() const
{
    return string("MATCHED NODE(") + *operands[0].as_str() + ")";
}

const string MatchedNode::cache_key(bool do_result_reduce) const
{
    //if(target == EXPLICIT)
    //    return "*"+*operands[0].as_str();
    return *operands[0].as_str();
}

const map<uint64_t, set<string> > MatchedNode::match(const Model &m, ModelGetter *getter){
    return map<uint64_t, set<string> >();
}


/* ErrorNode */

ErrorNode::ErrorNode(const string &info):
    HQLNode(ERR, NORMAL, IMPLICIT)
{
    etype = "bad-type(ErrorNode)";
    push_operand(HQLOperand(info));
}


ErrorNode::ErrorNode(const ErrorNode &n):
    HQLNode(n)
{
}

const ErrorNode& ErrorNode::operator=(const ErrorNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

ErrorNode::~ErrorNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* ErrorNode::reduce()
{
    return this->copy();
}

HQLNode* ErrorNode::copy()
{
    return new ErrorNode(*this);
}


const string ErrorNode::to_hql() const
{
    return string("ERROR NODE(") + *operands[0].as_str() + ")";
}

const string ErrorNode::cache_key(bool do_result_reduce) const
{
    return string("ERROR NODE(") + *operands[0].as_str() + ")";
}

const map<uint64_t, set<string> > ErrorNode::match(const Model &m, ModelGetter *getter){
    return map<uint64_t, set<string> >();
}


/* EachNode */

EachNode::EachNode(const string &type):
    HQLNode(EACH, NORMAL, NEGATIVE)
{
    etype = type;
    push_operand(HQLOperand(type));
}


EachNode::EachNode(const EachNode &n):
    HQLNode(n)
{
}

const EachNode& EachNode::operator=(const EachNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

EachNode::~EachNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* EachNode::reduce()
{
    return this->copy();
}

HQLNode* EachNode::copy()
{
    return new EachNode(*this);
}


const string EachNode::to_hql() const
{
    return "EACH";
}

const string EachNode::cache_key(bool do_result_reduce) const
{
    return "(EACH " + etype + ")";
}

const map<uint64_t, set<string> > EachNode::match(const Model &m, ModelGetter *getter){
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    uint64_t fn = m.fullname();
    string type = m.type();
    if(type == etype){
        ret[fn].insert("#"+num2str(fn));
    }
    return ret;
}


bool EachNode::validate() const
{
    return HQLNode::validate();
}


/* FullnameNode */

FullnameNode::FullnameNode(const uint64_t fn):
    HQLNode(FULLNAME, NORMAL, NEGATIVE)
{
    push_operand(HQLOperand(fn));
}


FullnameNode::FullnameNode(const FullnameNode &n):
    HQLNode(n)
{
}

const FullnameNode& FullnameNode::operator=(const FullnameNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

FullnameNode::~FullnameNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* FullnameNode::reduce()
{
    return this->copy();
}

HQLNode* FullnameNode::copy()
{
    return new FullnameNode(*this);
}


const string FullnameNode::to_hql() const
{
    std::ostringstream convert;
    convert << "#" << operands[0].as_fullname();
    return convert.str();
}

const string FullnameNode::cache_key(bool do_result_reduce) const
{
    return to_hql();
}

const map<uint64_t, set<string> > FullnameNode::match(const Model &m, ModelGetter *getter){
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    uint64_t fn = m.fullname();
    if(fn == operands[0].as_fullname()){
        ret[fn].insert("#"+num2str(fn));
    }
    return ret;
}

bool FullnameNode::validate() const
{
    uint8_t tid = operands[0].as_fullname() & 0xFF;
    map<uint8_t, TypeModel*>::iterator it = TypeConfig::rall_types.find(tid);
    if(it == TypeConfig::rall_types.end()){
        error_info = "bad fullname " + to_hql();
        return false;
    }
    etype = it->second->name;
    if(!HQLNode::validate()){
        return false;
    }
    return true;
}
