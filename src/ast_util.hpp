/* -*- c++ -*-
 *
 * file: ast_util.hpp
 * author: KDr2
 *
 */

#ifndef _AST_UTIL_HPP
#define _AST_UTIL_HPP

#define NDEBUG
#include "../vendor/libjson/libjson.h"
#undef  NDEBUG

#include "ast.hpp"
#include "model.hpp"

#include <inttypes.h>
#include <vector>
#include <functional>

using std::vector;

class ModelGetter;

class ASTUtil{
public:
    static HQLNode* parser_hql(const string &s);
    static Model get_model(uint64_t, ModelGetter*);
    static vector<uint64_t> get_relations(const string &rel_key, ModelGetter*);
};


class ModelGetter : public std::unary_function<uint64_t, const string>{
public:
    virtual const JSONNode operator()(uint64_t fn){return JSONNode();}
    virtual const vector<uint64_t> operator()(const string &rel_key){return vector<uint64_t>();}
    virtual ~ModelGetter(){};
};

/*
 * JSONModelGetter
 *
 */

class JSONModelGetter : public ModelGetter
{
public:
    JSONModelGetter(const JSONNode *_data):
        data(_data)
    {};

    const JSONNode operator()(uint64_t fn);
    const vector<uint64_t> operator()(const string &rel_key);

private:
    const JSONNode *data;
};



// HQLNodeConstMemFunCaller: call HQLNode const member function from HQLNode*/HQLOperands
template<typename N, typename R, R(HQLNode::*F)() const>
class HQLNodeConstMemFunCaller : public std::unary_function<N, R>{};

template<typename R, R(HQLNode::*F)() const>
class HQLNodeConstMemFunCaller<HQLNode*, R, F> : public std::unary_function<HQLNode*, R>{
public:
    R operator()(HQLNode *n){ return (n->*F)();}
};

template<typename R, R(HQLNode::*F)() const>
class HQLNodeConstMemFunCaller<HQLOperand, R, F> : public std::unary_function<HQLOperand, R>{
public:
    R operator()(const HQLOperand &n){ return (n.as_node().get()->*F)();}
};

///---- with 1 arg

template<typename N, typename R, typename A1, R(HQLNode::*F)(A1) const>
class HQLNodeConstMemFunCaller1 : public std::binary_function<N, A1, R>{};

template<typename R, typename A1, R(HQLNode::*F)(A1) const>
class HQLNodeConstMemFunCaller1<HQLNode*, R, A1, F> : public std::binary_function<HQLNode*, A1, R>{
public:
    R operator()(HQLNode *n, A1 a) const { return (n->*F)(a);}
};

template<typename R, typename A1, R(HQLNode::*F)(A1) const>
class HQLNodeConstMemFunCaller1<HQLOperand, R, A1, F> : public std::binary_function<HQLOperand, A1, R>{
public:
    R operator()(const HQLOperand &n, A1 a) const { return (n.as_node().get()->*F)(a);}
};


// HQLNodeMemFunCaller: call HQLNode member function from HQLNode*/HQLOperands
template<typename N, typename R, R(HQLNode::*F)()>
class HQLNodeMemFunCaller : public std::unary_function<N, R>{};

template<typename R, R(HQLNode::*F)()>
class HQLNodeMemFunCaller<HQLNode*, R, F> : public std::unary_function<HQLNode*, R>{
public:
    R operator()(HQLNode *n){ return (n->*F)();}
};

template<typename R, R(HQLNode::*F)()>
class HQLNodeMemFunCaller<HQLOperand, R, F> : public std::unary_function<HQLOperand, R>{
public:
    R operator()(const HQLOperand &n){ return (n.as_node().get()->*F)();}
};

// HQLNodeIsType: if a HQLNode*/HQLOperand 's Type is T
template<typename N, HQLNode::NODE_TYPE T>
class HQLNodeIsType : public std::unary_function<N, bool>{};

template<HQLNode::NODE_TYPE T>
class HQLNodeIsType<HQLNode*, T> : public std::unary_function<HQLNode*, bool>{
public:
    bool operator()(HQLNode *n){ return n->get_type() == T;}
};

template<HQLNode::NODE_TYPE T>
class HQLNodeIsType<HQLOperand, T> : public std::unary_function<HQLOperand, bool>{
public:
    bool operator()(const HQLOperand &n){ return n.as_node()->get_type() == T;}
};


// HQLNodeIsSubtype: if a HQLNode*/HQLOperand 's subtype is T
template<typename N, HQLNode::NODE_SUBTYPE T>
class HQLNodeIsSubtype : public std::unary_function<N, bool>{};

template<HQLNode::NODE_SUBTYPE T>
class HQLNodeIsSubtype<HQLNode*, T> : public std::unary_function<HQLNode*, bool>{
public:
    bool operator()(HQLNode *n){ return n->get_subtype() == T;}
};


template<HQLNode::NODE_SUBTYPE T>
class HQLNodeIsSubtype<HQLOperand, T> : public std::unary_function<HQLOperand, bool>{
public:
    bool operator()(const HQLOperand &n){ return n.as_node()->get_subtype() == T;}
};


// HQLNodeIsSubtype: if a HQLNode*/HQLOperand 's subtype is NOT T
template<typename N, HQLNode::NODE_SUBTYPE T>
class HQLNodeIsNotSubtype : public std::unary_function<N, bool>{};


template<HQLNode::NODE_SUBTYPE T>
class HQLNodeIsNotSubtype<HQLNode*, T> : public std::unary_function<HQLNode*, bool>{
public:
    bool operator()(HQLNode *n){ return n->get_subtype() != T;}
};


template<HQLNode::NODE_SUBTYPE T>
class HQLNodeIsNotSubtype<HQLOperand, T> : public std::unary_function<HQLOperand, bool>{
public:
    bool operator()(const HQLOperand &n){ return n.as_node()->get_subtype() != T;}
};


// CacheKey Compare
template<typename T>
class HQLNodeIsCacheKeyEquals
{
public:
    bool operator()(T a, T b){
        return false;
    }
};

template<>
class HQLNodeIsCacheKeyEquals<HQLOperand>
{
public:
    bool operator()(const HQLOperand &a, const HQLOperand &b){
        return a.as_node()->cache_key() == b.as_node()->cache_key();
    }
};


template<>
class HQLNodeIsCacheKeyEquals<shared_ptr<HQLNode> >
{
public:
    bool operator()(shared_ptr<HQLNode> a, shared_ptr<HQLNode> b){
        return a->cache_key() == b->cache_key();
    }
};


template<typename T>
class HQLNodeIsCacheKeyLess
{
public:
    bool operator()(T a, T b){
        return false;
    }
};

template<>
class HQLNodeIsCacheKeyLess<HQLOperand>
{
public:
    bool operator()(const HQLOperand &a, const HQLOperand &b){
        return a.as_node()->cache_key() < b.as_node()->cache_key();
    }
};

template<>
class HQLNodeIsCacheKeyLess<shared_ptr<HQLNode> >
{
public:
    bool operator()(shared_ptr<HQLNode> a, shared_ptr<HQLNode> b){
        return a->cache_key() < b->cache_key();
    }
};

// HQLOperand and JSONNode Comparer

template<typename F, COND_OPER::COND_OPER CO>
struct _CompareHQLOperandAndJSONNode{
    bool operator()(F l, F r){
        switch(CO){
        case COND_OPER::EQ:
            return std::equal_to<F>()(l, r);
        case COND_OPER::GT:
            return std::greater<F>()(l, r);
        case COND_OPER::LT:
            return std::less<F>()(l, r);
        case COND_OPER::GE:
            return std::greater_equal<F>()(l, r);
        case COND_OPER::LE:
            return std::less_equal<F>()(l, r);
        default:
            return false;
        }
    }
};

template<COND_OPER::COND_OPER CO>
bool CompareHQLOperandAndJSONNode(const HQLOperand &o,
                                  const JSONNode &n)
{
    HQLOperand::OPERAND_TYPE type = o.get_type();
    switch(type){
    case HQLOperand::BOOL:
        {
            if(n.type()!=JSON_BOOL){ return false; }
            return _CompareHQLOperandAndJSONNode<bool, CO>()(n.as_bool(), o.as_bool());
        }
    case HQLOperand::NUM:
        {
            if(n.type()!=JSON_NUMBER){ return false; }
            return _CompareHQLOperandAndJSONNode<double, CO>()(n.as_float(), o.as_num());
        }
    case HQLOperand::FULLNAME:
        {
            if(n.type()!=JSON_NUMBER){ return false; }
            return _CompareHQLOperandAndJSONNode<uint64_t, CO>()(n.as_int(), o.as_fullname());
        }
    case HQLOperand::STRING:
        {
            if(n.type()!=JSON_STRING){ return false; }
            string *s = o.as_str();
            string  j = string("\"") + n.as_string() + "\"";
            return _CompareHQLOperandAndJSONNode<string, CO>()(j, *s);
        }
    case HQLOperand::ID:
    case HQLOperand::NODE:
    case HQLOperand::COOP:
    case HQLOperand::NUM_ARRAY:
    case HQLOperand::STR_ARRAY:
    case HQLOperand::CONTAINS_ARG:
    case HQLOperand::KW:
    default:
        break;
    }
    return false;
}


#endif
