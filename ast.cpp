/* -*- c++ -*-
 *
 * file: ast.cpp
 * author: KDr2
 *
 */

#include "ast.hpp"
#include "cmn_util.hpp"
#include "ast_util.hpp"
#include "type_config.hpp"

#include <time.h>
#include <sstream>
#include <functional>

/* HQLOperand */


HQLOperand::HQLOperand(const KEYWORD k):
    type(KW)
{
    data.kw = k;
}

HQLOperand::HQLOperand(const uint64_t i):
    type(FULLNAME)
{
    data.fullname = i;
}

HQLOperand::HQLOperand(const number i):
    type(NUM)
{
    data.num = i;
}

HQLOperand::HQLOperand(COND_OPER::COND_OPER coop):
    type(COOP)
{
    data.coop = coop;
}

HQLOperand::HQLOperand(const string& s, OPERAND_TYPE ot):
    type(ot)
{
    data.str = new string(s);
}

HQLOperand::HQLOperand(const vector<string> *v, OPERAND_TYPE ot):
    type(ot)
{
    data.array = v;
}

HQLOperand::HQLOperand(shared_ptr<HQLNode> n_ptr):
    type(NODE)
{
    node = n_ptr;
}

HQLOperand::HQLOperand(const JSONNode &n, OPERAND_TYPE ot)
{
    // ot == ID -> auto
    if(ot == ID){
        switch(n.type()){
        case JSON_NULL:
            type = NIL;
            break;
        case JSON_STRING:
            {
                type = STRING;
                data.str = new string(n.as_string());
                break;
            }
        case JSON_NUMBER:
            {
                type = NUM;
                data.num = n.as_float();
                break;
            }
        case JSON_BOOL:
            {
                type = BOOL;
                data.boolean = n.as_bool();
                break;
            }
        case JSON_ARRAY:
        case JSON_NODE:
            {
                //TODO
                break;
            }
        default:
            type = NIL;
        }
    }else{


    }
}

HQLOperand::HQLOperand(const HQLOperand& o)
{
    type = o.type;
    switch(type){
    case BOOL:
        data.boolean = o.data.boolean;
        break;
    case NUM:
        data.num = o.data.num;
        break;
    case ID:
    case STRING:
        data.str = new string(*o.data.str);
        break;
    case NODE:
        node = o.node;
        break;
    case COOP:
        data.coop = o.data.coop;
        break;
    case NUM_ARRAY:
    case STR_ARRAY:
    case CONTAINS_ARG:
        data.array = new vector<string>(*o.data.array);
        break;
    case KW:
        data.kw=o.data.kw;
        break;
    case FULLNAME:
        data.fullname=o.data.fullname;
        break;
    default:
        break;
    }
}

HQLOperand::~HQLOperand(){
    switch(type){
        //case NUM: nothong todo
    case ID:
    case STRING:
        delete data.str;
        break;
    case NUM_ARRAY:
    case STR_ARRAY:
    case CONTAINS_ARG:
        delete data.array;
        break;
    case NODE:
        break;
    default:
        break;
    }
}

const HQLOperand& HQLOperand::operator=(const HQLOperand& rhs)
{
    if(this==&rhs) return *this;

    type = rhs.type;
    switch(type){
    case NUM:
        data.num = rhs.data.num;
        break;
    case ID:
    case STRING:
        delete data.str;
        data.str = new string(*rhs.data.str);
        break;
    case NODE:
        node=rhs.node;
        break;
    case COOP:
        data.coop = rhs.data.coop;
        break;
    case NUM_ARRAY:
    case STR_ARRAY:
    case CONTAINS_ARG:
        delete data.array;
        data.array = new vector<string>(*rhs.data.array);
        break;
    case KW:
        data.kw=rhs.data.kw;
        break;
    case FULLNAME:
        data.fullname=rhs.data.fullname;
        break;
    default:
        break;
    }
    return *this;
}

bool HQLOperand::operator<(const HQLOperand &rhs)
{
    if(this==&rhs) return false;
    if(this->type!=rhs.type){
        return this->type<rhs.type;
    }else{
        if(type == NODE){
            return this->as_node()->get_subtype() < rhs.as_node()->get_subtype();
        }
        return this < &rhs;
    }
    return true;
}

string HQLOperand::as_coop() const
{
    switch(data.coop){
    case COND_OPER::EQ:
        return "=";
        break;
    case COND_OPER::GT:
        return ">";
        break;
    case COND_OPER::LT:
        return "<";
        break;
    case COND_OPER::GE:
        return ">=";
        break;
    case COND_OPER::LE:
        return "<=";
        break;
    case COND_OPER::IN:
        return "IN";
        break;
    case COND_OPER::CONTAINS:
        return "CONTAINS";
        break;
    case COND_OPER::TIME_IN:
        return "TIME_IN";
        break;
    default:
        return "BAD-COOP";
        break;
    }
}

static string array2str(const vector<string> *v)
{
    std::ostringstream convert;
    convert<<"[";
    vector<string>::const_iterator it=v->begin();
    size_t s = v->size();
    if(s > 1){
        vector<string>::const_iterator realit = it++;
        while(it!=v->end()){
            convert<< *realit <<", ";
            it++;
            realit++;
        }
        convert<< *realit;
    }else if(s == 1){
        convert<< *it;
    }
    convert<<"]";
    return convert.str();
}


string HQLOperand::as_code_hql() const
{
    switch(type){
    case ID:
        return *data.str;
        break;
    case BOOL:
        return data.boolean? "true" : "false";
        break;
    case NUM:
        return num2str(data.num);
        break;
    case STRING:
        return *data.str;
        break;
    case NUM_ARRAY:
    case STR_ARRAY:
        return array2str(data.array);
        break;
    case CONTAINS_ARG:
        return ((*data.array)[0].size()==1 && (*data.array)[0][0] == 0) ?
            "EACH BY " + (*data.array)[1] :
            (*data.array)[0] + " BY " +(*data.array)[1];
        break;
    case NODE:
        return node->to_hql();
        break;
    case COOP:
        return as_coop();
        break;
    case KW:
        switch(data.kw){
        case AUTO:
            return "AUTO";
            break;
        case TAUTO:
            return "*AUTO";
            break;
        case EACH:
            return "EACH";
            break;
        default:
            return "BAD-KW";
        }
        break;
    case FULLNAME:
        return "#" + num2str(data.fullname);
        break;
    default:
        break;
    }
    return "BAD-OBJECT";
}

bool HQLOperand::predicate(const HQLOperand &o, const JSONNode &n)
{
    if(type != COOP){
        return false;
    }

    switch(data.coop){
    case COND_OPER::EQ:
        return CompareHQLOperandAndJSONNode<COND_OPER::EQ>(o, n);
        break;
    case COND_OPER::GT:
        return CompareHQLOperandAndJSONNode<COND_OPER::GT>(o, n);
        break;
    case COND_OPER::LT:
        return CompareHQLOperandAndJSONNode<COND_OPER::LT>(o, n);
        break;
    case COND_OPER::GE:
        return CompareHQLOperandAndJSONNode<COND_OPER::GE>(o, n);
        break;
    case COND_OPER::LE:
        return CompareHQLOperandAndJSONNode<COND_OPER::LE>(o, n);
        break;
    case COND_OPER::IN:
        {
            if(o.get_type()==NUM_ARRAY){
                if(n.type()==JSON_NUMBER){
                    string ns=num2str(n.as_int());
                    vector<string>::const_iterator it=o.as_str_array()->begin();
                    for(;it!=o.as_str_array()->end();it++){
                        if(ns==*it)return true;
                    }
                    return false;
                }
                return false;
            }else if(o.get_type()==STR_ARRAY){
                if(n.type()==JSON_STRING){
                    string ns=n.as_string();
                    vector<string>::const_iterator it=o.as_str_array()->begin();
                    for(;it!=o.as_str_array()->end();it++){
                        if(ns==*it)return true;
                    }
                    return false;
                }
                return false;
            }
            return false;
            break;
        }
    case COND_OPER::CONTAINS:
        {
            string val = o.as_str_array()->at(0);
            string sep = o.as_str_array()->at(1);
            string ns=n.as_string();
            string unit;
            string::size_type start = 0, end = 0;
            while(end!=string::npos){
                end = ns.find_first_of(sep, start);
                if(end==string::npos){
                    unit = ns.substr(start);
                }else{
                    unit = ns.substr(start, end-start);
                }
                start = end + 1;
                if(val == "\"" + unit + "\""){
                    return true;
                }
            }
            return false;
            break;
        }
    case COND_OPER::TIME_IN:
        {
            if(n.type()!=JSON_NUMBER) return false;
            if(o.get_type()!=NUM) return false;
            time_t now = time(NULL);
            return now-o.as_num() <= n.as_float();
            break;
        }
    default:
        return false;
    }
    return false;
}


/* HQLNode */

HQLNode::HQLNode(NODE_TYPE type, NODE_SUBTYPE stype, TARGET_TYPE target):
    type(type), subtype(stype), target(target)
{
}

HQLNode::HQLNode(const HQLNode& o):
    type(o.type), subtype(o.subtype), target(o.target)
{
    etype = o.etype;
    atype = o.atype;
    noperand = o.noperand;
    operands = o.operands;
}


HQLNode::~HQLNode(){
}

const HQLNode& HQLNode::operator=(const HQLNode& rhs)
{
    if(this == &rhs) return *this;
    etype = rhs.etype;
    type = rhs.type;
    subtype = rhs.subtype;
    target = rhs.target;
    noperand = rhs.noperand;
    operands = rhs.operands;
    return *this;
}

void HQLNode::push_operand(const HQLOperand& oper)
{
    operands.push_back(oper);
}

void HQLNode::push_operand(const vector<HQLOperand> &opers)
{
    std::back_insert_iterator< vector<HQLOperand> > backit(operands);
    std::copy(opers.begin(), opers.end(), backit);
}


bool HQLNode::validate() const
{
    if(!TypeConfig::has_type(etype)){
        error_info = "bad type";
        return false;
    }
    return true;
}
