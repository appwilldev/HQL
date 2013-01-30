/* -*- c++ -*-
 *
 * file: ast_sl.cpp
 * author: KDr2
 *
 */


#include "concrete_ast.hpp"
#include "type_config.hpp"

/* SLAllNode */

SLAllNode::SLAllNode(const string &type, TARGET_TYPE t):
    HQLNode(SL, ALL, t)
{
    etype = type;
    push_operand(HQLOperand(type));
}


SLAllNode::SLAllNode(const SLAllNode &n):
    HQLNode(n)
{
}

const SLAllNode& SLAllNode::operator=(const SLAllNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

SLAllNode::~SLAllNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* SLAllNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }
    return this->copy();
}

HQLNode* SLAllNode::copy()
{
    return new SLAllNode(*this);
}



const string SLAllNode::to_hql() const
{
    string prefix = target==EXPLICIT ? "*" : "";
    return string("SELECT ") + prefix + *operands[0].as_str();
}


const string SLAllNode::cache_key(bool do_result_reduce) const
{
    return "a[" + etype + "]";
}

const set<string> SLAllNode::get_ctypes() const
{
    return TypeConfig::get_subtypes(*operands[0].as_str());
}

const map<uint64_t, set<string> > SLAllNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    uint64_t fn = m.fullname();
    string type = m.type();
    if(TypeConfig::is_subtype(type, *operands[0].as_str())){
        if(TypeConfig::type_id(get_etype())==0){
            fn = FULLNAME_SET_CAT(fn);
        }
        ret[fn].insert(this->cache_key());
    }
    return ret;
}

bool SLAllNode::validate() const
{
    if(!HQLNode::validate()){
        return false;
    }
    return true;
}

/* SLCondNode */

SLCondNode::SLCondNode(const string &type,
                       const string &attr,
                       const HQLOperand &oper,
                       const HQLOperand &value,
                       TARGET_TYPE t):
    HQLNode(SL, COND, t)
{
    etype = type;
    push_operand(HQLOperand(type));
    push_operand(HQLOperand(attr));
    push_operand(oper);
    push_operand(value);
}


SLCondNode::SLCondNode(const SLCondNode &n):
    HQLNode(n)
{
}

const SLCondNode& SLCondNode::operator=(const SLCondNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

SLCondNode::~SLCondNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* SLCondNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }
    return copy();
}

HQLNode* SLCondNode::copy()
{
    return new SLCondNode(*this);
}

const string SLCondNode::to_hql() const
{
    string prefix = target==EXPLICIT ? "*" : "";
    return string("SELECT ") + prefix + *operands[0].as_str() +
        string(" WHERE ") + *operands[1].as_str() + string(" ") +
        operands[2].as_coop() + string(" ") + operands[3].as_code_hql();
}

const string SLCondNode::cache_key(bool do_result_reduce) const
{
    return "c[" + etype + "." + *operands[1].as_str() + string(",") +
        operands[2].as_coop() + string(",") + operands[3].as_code_hql() + "]";
}

const set<string> SLCondNode::get_ctypes() const{
    return TypeConfig::get_subtypes(*operands[0].as_str());
}

const map<uint64_t, set<string> > SLCondNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    uint64_t fn = m.fullname();
    string type = m.type();

    string attr = *operands[1].as_str();
    JSONNode v = m.attr<JSONNode>(attr);
    if(v.type()==JSON_NODE && v.size()<=0){
        return ret;
    }
    if(!TypeConfig::is_subtype(type, etype)){ // this will not happens
        return ret;
    }

    if(TypeConfig::type_id(get_etype())==0){
        fn = FULLNAME_SET_CAT(fn);
    }

    if(this->has_semantic_each()){
        switch(operands[2].as_coop_type()){
        case COND_OPER::EQ:
            {
                HQLOperand o = HQLOperand(v);
                SLCondNode n = SLCondNode(etype,
                                          attr,
                                          operands[2],
                                          o);
                ret[fn].insert(n.cache_key());
                break;
            }
        case COND_OPER::CONTAINS:
            {
                string sep = operands[3].as_str_array()->at(1);
                string val = v.as_string();
                string unit;
                string::size_type start = 0, end = 0;
                while(end!=string::npos){
                    end = val.find_first_of(sep, start);
                    if(end==string::npos){
                        unit = val.substr(start);
                    }else{
                        unit = val.substr(start, end-start);
                    }
                    start = end + 1;
                    if(unit.size()<1) continue;
                    vector<string> *arg = new vector<string>;//({unit, sep});
                    arg->push_back("\"" + unit + "\"");
                    arg->push_back(sep);

                    HQLOperand o = HQLOperand(arg, HQLOperand::CONTAINS_ARG);

                    SLCondNode n = SLCondNode(etype,
                                              attr,
                                              operands[2],
                                              o);
                    ret[fn].insert(n.cache_key());
                }
                break;
            }
        default:
            {
                //this will not happens
                return ret;
            }
        }
    }else{
        if(operands[2].predicate(operands[3], v)){
            ret[fn].insert(this->cache_key());
        }
    }

    return ret;
}


bool SLCondNode::validate() const
{
    if(!HQLNode::validate()){
        return false;
    }
    if(operands[3].get_type() == HQLOperand::KW &&
       operands[3].as_keyword() == ::EACH &&
       operands[2].as_coop_type() != COND_OPER::EQ){
        error_info = "can not use EACH as (non-eq)'s operand";
        return false;
    }
    return true;
}

bool SLCondNode::has_semantic_each() const
{
    if(operands[3].get_type() == HQLOperand::KW){
        return operands[3].as_keyword() == ::EACH;
    }
    if(operands[3].get_type() == HQLOperand::CONTAINS_ARG){
        string a1 = (operands[3].as_str_array())->at(0);
        return a1.size()==1 && a1[0] == 0;
    }
    return false;
}

pair<string, uint64_t> SLCondNode::time_in() const
{
    if(operands[2].as_coop_type() != COND_OPER::TIME_IN || has_semantic_each()){
        return std::make_pair("", 0UL);
    }
    return std::make_pair(*operands[1].as_str(), static_cast<uint64_t>(operands[3].as_num()));
}
