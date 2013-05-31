/* -*- c++ -*-
 *
 * file: ast_slfk.cpp
 * author: KDr2
 *
 */

#include "concrete_ast.hpp"
#include "type_config.hpp"

/* SLFKNode */

SLFKAllNode::SLFKAllNode(const string &type,
                         const string &fk,
                         const string &fkt,
                         TARGET_TYPE t):
    HQLNode(SLFK, ALL, t)
{
    etype = fkt;
    atype = type;
    push_operand(HQLOperand(type));
    push_operand(HQLOperand(fk));
    push_operand(HQLOperand(fkt));
}


SLFKAllNode::SLFKAllNode(const SLFKAllNode &n):
    HQLNode(n)
{
}

const SLFKAllNode& SLFKAllNode::operator=(const SLFKAllNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

SLFKAllNode::~SLFKAllNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* SLFKAllNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }
    return this->copy();
}

HQLNode* SLFKAllNode::copy()
{
    return new SLFKAllNode(*this);
}


const string SLFKAllNode::to_hql() const
{
    string prefix = target==EXPLICIT ? "*" : "";
    return string("SELECT ") + *operands[0].as_str() + string(".") +
        *operands[1].as_str() + string(" AS ") + prefix + *operands[2].as_str();
}

const string SLFKAllNode::cache_key(bool do_result_reduce) const
{
    return "A[" + *operands[0].as_str() + string(".") +
        *operands[1].as_str() + "->" + *operands[2].as_str() +"]";
}

const set<string> SLFKAllNode::get_ctypes() const{
    return TypeConfig::get_subtypes(*operands[0].as_str());
}

const map<uint64_t, set<string> > SLFKAllNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    //uint64_t fn = m.fullname();
    uint64_t gn = m.attr<uint64_t>(*operands[1].as_str());
    string type = m.type();
    if(TypeConfig::is_subtype(type, *operands[0].as_str())){
        if(TypeConfig::type_id(*operands[0].as_str())==0){
            gn = FULLNAME_SET_CAT(gn);
        }
        ret[gn].insert(this->cache_key());
    }
    return ret;
}


bool SLFKAllNode::validate() const
{
    if(!HQLNode::validate()){
        return false;
    }
    if(!TypeConfig::has_type(*operands[0].as_str())){
        error_info = "bad fk-host type";
        return false;
    }
    return true;
}

vector<string> SLFKAllNode::has_semantic_fk() const
{
    vector<string> ret;
    ret.push_back(get_etype());
    ret.push_back(get_atype());
    return ret;
}


/* SLFKCondNode */


SLFKCondNode::SLFKCondNode(const string &type,
                           const string &fk,
                           const string &fkt,
                           const string &attr,
                           const HQLOperand &oper,
                           const HQLOperand &value,
                           TARGET_TYPE t):
    HQLNode(SLFK, COND, t)
{
    etype = fkt;
    atype = type;
    push_operand(HQLOperand(type));
    push_operand(HQLOperand(fk));
    push_operand(HQLOperand(fkt));
    push_operand(HQLOperand(attr));
    push_operand(oper);
    push_operand(value);
}


SLFKCondNode::SLFKCondNode(const SLFKCondNode &n):
    HQLNode(n)
{
}

const SLFKCondNode& SLFKCondNode::operator=(const SLFKCondNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

SLFKCondNode::~SLFKCondNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* SLFKCondNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }
    return copy();
}

HQLNode* SLFKCondNode::copy()
{
    return new SLFKCondNode(*this);
}

const string SLFKCondNode::to_hql() const
{
    string prefix = target==EXPLICIT ? "*" : "";
    return string("SELECT ") + *operands[0].as_str() + string(".") +
        *operands[1].as_str() + string(" AS ") + prefix + *operands[2].as_str() +
        string(" WHERE ") + *operands[3].as_str() + string(" ") +
        operands[4].as_coop() + string(" ") + operands[5].as_code_hql();
}

const string SLFKCondNode::cache_key(bool do_result_reduce) const
{
    return "C[" + *operands[0].as_str() + string(".") +
        *operands[1].as_str() + "->" + *operands[2].as_str() +
        "," + *operands[3].as_str() + string(",") +
        operands[4].as_coop() + string(",") + operands[5].as_code_hql() +
        "]";
}

const set<string> SLFKCondNode::get_ctypes() const{
    return TypeConfig::get_subtypes(*operands[0].as_str());
}


const map<uint64_t, set<string> > SLFKCondNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret;
    if(m.attr<bool>("deleted")) return ret;
    //uint64_t fn = m.fullname();
    uint64_t gn = m.attr<uint64_t>(*operands[1].as_str());
    string type = m.type();

    string attr = *operands[3].as_str();
    JSONNode v = m.attr<JSONNode>(attr);
    if(v.type()==JSON_NODE && v.size()<=0){
        return ret;
    }

    if(!TypeConfig::is_subtype(type, *operands[0].as_str())){
        // this will not happens
        return ret;
    }

    if(TypeConfig::type_id(*operands[0].as_str())==0){
        gn = FULLNAME_SET_CAT(gn);
    }

    if(this->has_semantic_each()){
        switch(operands[4].as_coop_type()){
        case COND_OPER::EQ:
            {
                HQLOperand o = HQLOperand(v);
                SLFKCondNode n = SLFKCondNode(type,
                                              *operands[1].as_str(),
                                              etype,
                                              attr,
                                              operands[4],
                                              o);
                ret[gn].insert(n.cache_key());
                break;
            }
        case COND_OPER::CONTAINS:
            {
                string sep = operands[5].as_str_array()->at(1);
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

                    SLFKCondNode n = SLFKCondNode(type,
                                                  *operands[1].as_str(),
                                                  etype,
                                                  attr,
                                                  operands[4],
                                                  o);
                    ret[gn].insert(n.cache_key());
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
        if(operands[4].predicate(operands[5], v)){
            ret[gn].insert(this->cache_key());
        }
    }

    return ret;
}


bool SLFKCondNode::validate() const
{
    if(!HQLNode::validate()){
        return false;
    }
    if(!TypeConfig::has_type(*operands[0].as_str())){
        error_info = "bad fk-host type";
        return false;
    }
    if(operands[5].get_type() == HQLOperand::KW &&
       operands[5].as_keyword() == ::EACH &&
       operands[4].as_coop_type() != COND_OPER::EQ){
        error_info = "can not use EACH as (non-eq)'s operand";
        return false;
    }
    return true;
}
bool SLFKCondNode::has_semantic_each() const
{
    if(operands[5].get_type() == HQLOperand::KW){
        return operands[5].as_keyword() == ::EACH;
    }
    if(operands[5].get_type() == HQLOperand::CONTAINS_ARG){
        string a1 = (operands[5].as_str_array())->at(0);
        return a1.size()==1 && a1[0] == 0;
    }
    return false;
}

pair<string, uint64_t> SLFKCondNode::time_in() const
{
    if(operands[4].as_coop_type() != COND_OPER::TIME_IN || has_semantic_each()){
        return std::make_pair("", 0UL);
    }
    return std::make_pair(*operands[3].as_str(), static_cast<uint64_t>(operands[5].as_num()));
}


vector<string> SLFKCondNode::has_semantic_fk() const
{
    vector<string> ret;
    ret.push_back(get_etype());
    ret.push_back(get_atype());
    return ret;
}
