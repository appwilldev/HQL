/* -*- c++ -*-
 *
 * file: ast.hpp
 * author: KDr2
 *
 */

#ifndef _AST_HPP
#define _AST_HPP

#include <inttypes.h>

#include <iostream>
#include <string>
#include <map>
#include <set>
#include <vector>
#include <memory>
#include <utility>
#include <tr1/memory>

#define NDEBUG
#include "../vendor/libjson/libjson.h"
#undef NDEBUG

#include "model.hpp"

using std::set;
using std::map;
using std::pair;
using std::string;
using std::vector;
using std::tr1::shared_ptr;

class ModelGetter;

namespace COND_OPER{
    enum COND_OPER{
        EQ, GT, LT, GE, LE,
        IN, CONTAINS, TIME_IN
    };
}

enum KEYWORD{
    AUTO, TAUTO, EACH,
};

class HQLNode;

class HQLOperand{
public:

    typedef double number;

    typedef enum{
        ID, KW, FULLNAME, COOP,
        NIL, BOOL, NUM, STRING,
        NUM_ARRAY, STR_ARRAY, CONTAINS_ARG,
        NODE,
    }OPERAND_TYPE;

    HQLOperand():type(KW){ data.kw=AUTO;};
    HQLOperand(const bool b):type(BOOL){ data.boolean=b;};
    HQLOperand(const KEYWORD k);
    HQLOperand(const uint64_t i);
    HQLOperand(const number i);
    HQLOperand(COND_OPER::COND_OPER coop);
    HQLOperand(const string&, OPERAND_TYPE ot=ID);
    HQLOperand(const vector<string>*, OPERAND_TYPE ot=NUM_ARRAY);
    HQLOperand(shared_ptr<HQLNode>);
    HQLOperand(const JSONNode&, OPERAND_TYPE ot=ID);
    HQLOperand(const HQLOperand&);
    virtual ~HQLOperand();

    const HQLOperand& operator=(const HQLOperand&);
    bool operator<(const HQLOperand&);
    OPERAND_TYPE get_type() const {return type;};
    number as_num() const {return data.num;}
    bool as_bool() const {return data.boolean;}
    uint64_t as_fullname() const {return data.fullname;}
    KEYWORD as_keyword() const {return data.kw;}
    string* as_str() const {return data.str;}
    const vector<string>* as_str_array() const {return data.array;}
    shared_ptr<HQLNode> as_node() const {return node;}
    string as_coop() const;
    COND_OPER::COND_OPER as_coop_type() const {return data.coop;}
    string as_code_hql() const;
    bool predicate(const HQLOperand&, const JSONNode&);


private:

    OPERAND_TYPE type;
    union{
        bool boolean;
        number num;
        KEYWORD kw;
        uint64_t fullname;
        COND_OPER::COND_OPER coop;
        string *str;
        const vector<string> *array;
    }data;
    shared_ptr<HQLNode> node;
};

class HQLNode{
public:
    typedef enum{
        ERR, EACH, FULLNAME, SL, SLFK, RL, LOGIC, MISC,
    } NODE_TYPE;

    typedef enum{
        NORMAL,
        ALL, COND, /* SL, SLFK */
        AND, OR, NOT, /* LOGIC */
        LIMIT, ORDER_BY, /* MISC */
    }NODE_SUBTYPE;

    typedef enum{
        NEGATIVE, IMPLICIT, EXPLICIT,
    }TARGET_TYPE;


    HQLNode(NODE_TYPE, NODE_SUBTYPE=NORMAL, TARGET_TYPE target=IMPLICIT);
    HQLNode(const HQLNode&);
    virtual ~HQLNode();

    const HQLNode& operator=(const HQLNode&);

    void push_operand(const HQLOperand&);
    void push_operand(const vector<HQLOperand>&);
    void set_etype(const string& t){etype = t;}
    void set_target(TARGET_TYPE tt){target = tt;}
    TARGET_TYPE get_target() const {return target;}
    void set_noperand(uint16_t n){noperand = n;}
    bool error() const {return type == ERR;}
    NODE_TYPE get_type() const {return type;}
    const string& get_etype() const {return etype;}
    const string& get_atype() const {return atype;}
    NODE_SUBTYPE get_subtype() const {return subtype;}
    const HQLOperand& get_operand(size_t i) const {return operands[i];}
    const vector<HQLOperand>& get_operands() const {return operands;}
    virtual const string to_hql() const = 0;
    virtual const string cache_key(bool do_result_reduce=false) const = 0;
    virtual const set<string> get_ctypes() const = 0;
    virtual const set<string> get_rtypes() const = 0;
    virtual ExtraMatchDataInfo get_xmdinfo() const = 0;
    virtual const map<uint64_t, set<string> > match(const Model &m, ModelGetter *getter) = 0;
    virtual HQLNode* copy() = 0;
    virtual HQLNode* reduce() = 0;
    virtual bool validate() const;
    virtual bool has_semantic_each() const = 0;
    virtual pair<string, uint64_t> time_in() const {return std::make_pair("",0UL);};
    virtual vector<string> has_semantic_fk() const = 0;

protected:
    NODE_TYPE type;
    NODE_SUBTYPE subtype;
    TARGET_TYPE target;
    mutable string etype; /** element type of this list */
    mutable string atype; /** auxilary type of this node (host type in FKNode)/*/
    uint16_t noperand; /** num of operands; unused */
    vector<HQLOperand> operands;
    mutable string error_info;
};



#endif /* _AST_HPP */
