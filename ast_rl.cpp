/* -*- c++ -*-
 *
 * file: ast_rl.cpp
 * author: KDr2
 *
 */

#include "concrete_ast.hpp"
#include "cmn_util.hpp"
#include "ast_util.hpp"
#include "type_config.hpp"

/* RLNode */

RLNode::RLNode(const HQLOperand &rel,
               const HQLOperand &left,
               const HQLOperand &right,
               TARGET_TYPE t):
    HQLNode(RL, NORMAL, t)
{
    push_operand(rel);
    push_operand(left);
    push_operand(right);
}


RLNode::RLNode(const RLNode &n):
    HQLNode(n)
{
}

const RLNode& RLNode::operator=(const RLNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

RLNode::~RLNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* RLNode::reduce()
{

    HQLNode *n0=NULL, *n1=NULL, *n2=NULL;
    if(operands[0].get_type() == HQLOperand::NODE){
        n0 = operands[0].as_node()->reduce();
    }else{
        return new ErrorNode("Relation can't be AUTO");
    }


    if(operands[1].get_type() == HQLOperand::NODE){
        n1 = operands[1].as_node()->reduce();
    }else if(operands[1].get_type() == HQLOperand::KW){
        string type = TypeConfig::left_type(n0->get_etype());
        //check type
        if(type.size()<1) type = "bad-type";
        switch(operands[1].as_keyword()){
        case AUTO:
            n1 = new SLAllNode(type, IMPLICIT);
            break;
        case TAUTO:
            n1 = new SLAllNode(type, EXPLICIT);
            break;
        case ::EACH:
            n1 = new EachNode(type);
            break;
        }
    }else if(operands[1].get_type() == HQLOperand::FULLNAME){
        n1 = new FullnameNode(operands[1].as_fullname());
    }

    if(operands[2].get_type() == HQLOperand::NODE){
        n2 = operands[2].as_node()->reduce();
    }else if(operands[2].get_type() == HQLOperand::KW){
        string type = TypeConfig::right_type(n0->get_etype());
        // check type
        if(type.size()<1) type = "bad-type";
        switch(operands[2].as_keyword()){
        case AUTO:
            n2 = new SLAllNode(type, IMPLICIT);
            break;
        case TAUTO:
            n2 = new SLAllNode(type, EXPLICIT);
            break;
        case ::EACH:
            n2 = new EachNode(type);
            break;
        }
    }else if(operands[2].get_type() == HQLOperand::FULLNAME){
        n2 = new FullnameNode(operands[2].as_fullname());
    }

    RLNode *ret = new RLNode(HQLOperand(shared_ptr<HQLNode>(n0)),
                             HQLOperand(shared_ptr<HQLNode>(n1)),
                             HQLOperand(shared_ptr<HQLNode>(n2)));
    if(!ret->validate()){
        string hql = ret->to_hql();
        delete ret;
        return new ErrorNode("bad RL: " + hql);
    }
    return ret;
}

HQLNode* RLNode::copy()
{
    return new RLNode(*this);
}


const string RLNode::to_hql() const
{
    return string("(") + operands[0].as_code_hql() + string(") BETWEEN (") +
        operands[1].as_code_hql() + string(") AND (") +
        operands[2].as_code_hql() + string(")");
}

const string RLNode::cache_key(bool do_result_reduce) const
{
    if(operands[0].get_type() == HQLOperand::NODE &&
       operands[1].get_type() == HQLOperand::NODE &&
       operands[2].get_type() == HQLOperand::NODE){
        string decos[]={"r[","<",",",">]"};
        for(int i=0;i<3;i++){
            if(operands[i].as_node()->get_target()==EXPLICIT){
                decos[i]=decos[i]+"*";
                break;
            }
        }
        return decos[0] + operands[0].as_node()->cache_key() + decos[1] +
            operands[1].as_node()->cache_key() + decos[2] +
            operands[2].as_node()->cache_key() + decos[3];
    }else{
        return "unReducedRL(" + to_hql() +")";
    }
}

const set<string> RLNode::get_ctypes() const{
    return this->get_rtypes();
}

ExtraMatchDataInfo RLNode::get_xmdinfo() const
{
    ExtraMatchDataInfo ret;
    ret.relation_info = 1;

    if(!(HQLNodeIsType<HQLOperand, EACH>()(operands[1]) ||
         (HQLNodeIsType<HQLOperand, SL>()(operands[1]) && HQLNodeIsSubtype<HQLOperand, ALL>()(operands[1])))){
        ret.add_key("left");
    }
    if(!(HQLNodeIsType<HQLOperand, EACH>()(operands[2]) ||
         (HQLNodeIsType<HQLOperand, SL>()(operands[2]) && HQLNodeIsSubtype<HQLOperand, ALL>()(operands[2])))){
        ret.add_key("right");
    }
    return ret;
}

const set<string> RLNode::get_rtypes() const{
    set<string> ret = operands[0].as_node()->get_ctypes();
    if(operands[1].get_type() == HQLOperand::NODE){
        set<string> ext = operands[1].as_node()->get_rtypes();
        ret.insert(ext.begin(), ext.end());
    }
    if(operands[2].get_type() == HQLOperand::NODE){
        set<string> ext = operands[2].as_node()->get_rtypes();
        ret.insert(ext.begin(), ext.end());
    }
    return ret;
}

const map<uint64_t, set<string> > RLNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, rel, l, r;
    if(m.attr<bool>("deleted")) return ret;
    rel = operands[0].as_node()->match(m, getter);
    uint64_t fn_m = m.fullname();
    uint64_t fn_l = m.left();
    uint64_t fn_r = m.right();
    uint64_t target = 0 ;
    if(operands[0].as_node()->get_target() == HQLNode::EXPLICIT){
        target = fn_m;
    }else if(operands[1].as_node()->get_target() == HQLNode::EXPLICIT){
        target = fn_l;
    }else if(operands[2].as_node()->get_target() == HQLNode::EXPLICIT){
        target = fn_r;
    }
    if(target != fn_m){
        vector<uint64_t> rels = ASTUtil::get_relations(m.rel_cache_key(), getter);
        if(rels.size()>1 or (rels.size()==1 && rels[0] != m.fullname())){
            // has other relations and the target is left/right
            return ret;
        }
    }
    if(HQLNodeIsType<HQLOperand, EACH>()(operands[1])){
        l[fn_l].insert("#" + num2str(fn_l));
    }else if(HQLNodeIsType<HQLOperand, SL>()(operands[1]) && HQLNodeIsSubtype<HQLOperand, ALL>()(operands[1])){
        l[fn_l].insert(operands[1].as_node()->cache_key());
    }else{
        l = operands[1].as_node()->match(ASTUtil::get_model(fn_l, getter), getter);
    }

    if(HQLNodeIsType<HQLOperand, EACH>()(operands[2])){
        r[fn_r].insert("#" + num2str(fn_r));
    }else if(HQLNodeIsType<HQLOperand, SL>()(operands[2]) && HQLNodeIsSubtype<HQLOperand, ALL>()(operands[2])){
        r[fn_r].insert(operands[2].as_node()->cache_key());
    }else{
        r = operands[2].as_node()->match(ASTUtil::get_model(fn_r, getter), getter);
    }

    set<string>::iterator it_rel, it_l, it_r;
    for(it_rel = rel[fn_m].begin(); it_rel!=rel[fn_m].end(); it_rel++){
        for(it_l = l[fn_l].begin(); it_l!=l[fn_l].end(); it_l++){
            for(it_r = r[fn_r].begin(); it_r!=r[fn_r].end(); it_r++){
                RLNode ret_node(HQLOperand(shared_ptr<HQLNode>(new MatchedNode(*it_rel,
                                                                               operands[0].as_node()->get_target()))),
                                (HQLOperand(shared_ptr<HQLNode>(new MatchedNode(*it_l,
                                                                                operands[1].as_node()->get_target())))),
                                (HQLOperand(shared_ptr<HQLNode>(new MatchedNode(*it_r,
                                                                                operands[2].as_node()->get_target())))));
                ret[target].insert(ret_node.cache_key());
            }
        }
    }

    return ret;
}

bool RLNode::validate() const
{
    bool ret = operands[0].as_node()->validate();
    if(!ret){
        error_info = "sub statement error";
        return false;
    }

    string rel_type = operands[0].as_node()->get_etype();
    map<string, TypeModel*>::iterator it = TypeConfig::all_types.find(rel_type);
    if(it == TypeConfig::all_types.end()){
        error_info = "bad relation type: " + rel_type;
        return false;
    }
    if(it->second->id<=128){
        error_info = "bad relation type: " + rel_type;
        return false;
    }

    // Check Left and Right

    HQLOperand tmp_operands[3];
    tmp_operands[0] = operands[0];

    string tmp_types[3];
    // tmp_types[0] = rel_type; // no need
    tmp_types[1] = TypeConfig::left_type(rel_type);
    tmp_types[2] = TypeConfig::right_type(rel_type);
    for(int i=1; i<3; i++){
        switch(operands[i].get_type()){
        case HQLOperand::KW:
            switch(operands[i].as_keyword()){
            case ::AUTO:
                tmp_operands[i] = HQLOperand(shared_ptr<HQLNode>(new SLAllNode(tmp_types[i], IMPLICIT)));
                break;
            case ::TAUTO:
                tmp_operands[i] = HQLOperand(shared_ptr<HQLNode>(new SLAllNode(tmp_types[i], EXPLICIT)));
                break;
            case ::EACH:
                tmp_operands[i] = HQLOperand(shared_ptr<HQLNode>(new EachNode(tmp_types[i])));
            break;
            }
            break;
        case HQLOperand::FULLNAME:
            tmp_operands[i] = HQLOperand(shared_ptr<HQLNode>(new FullnameNode(operands[i].as_fullname())));
            break;
        case HQLOperand::NODE:
            tmp_operands[i] = operands[i];
            break;
        default:
            error_info = "left/right statement error";
            return false;
        }
    }

    // check TARGET_TYPE
    int i=0, j=0, k=0, t=-1;
    for(i=0; i<3; i++){
        if(i==0){j=1; k=2;}
        if(i==1){j=0; k=2;}
        if(i==2){j=0; k=1;}
        if(tmp_operands[i].as_node()->get_target() == EXPLICIT){
            if(tmp_operands[j].as_node()->get_target() == EXPLICIT ||
               tmp_operands[k].as_node()->get_target() == EXPLICIT) {
                error_info = "more than one EXPLICIT node";
                return false;
            }
            t = i;
            etype = tmp_operands[i].as_node()->get_etype();
        }
    }
    if(t<0){
        error_info = "no IMPLICIT etype in statement: " + to_hql();
        return false;
    }
    return true;
}


bool RLNode::has_semantic_each() const
{
    HQLNode *n=NULL;
    for(int i=0; i<3; i++){
        if(operands[i].get_type() == HQLOperand::NODE){
            n = operands[i].as_node()->reduce();
            bool ret = n->has_semantic_each();
            delete n;
            if(ret){
                return ret;
            }
        }else if(operands[i].get_type() == HQLOperand::KW && operands[i].as_keyword() == ::EACH){
            return true;
        }
    }
    return false;
}
