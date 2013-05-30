/* -*- c++ -*-
 *
 * file: ast_logic.cpp
 * author: KDr2
 *
 */


#include "cmn_util.hpp"
#include "ast_util.hpp"
#include "type_config.hpp"
#include "concrete_ast.hpp"

#include <iterator>
#include <algorithm>
#include <functional>
#include <cstdio>

/* LogicAndNode */

LogicAndNode::LogicAndNode(shared_ptr<HQLNode> l,
                           shared_ptr<HQLNode> r,
                           TARGET_TYPE t):
    HQLNode(LOGIC, AND, t)
{
    push_operand(HQLOperand(l));
    push_operand(HQLOperand(r));
}

LogicAndNode::LogicAndNode(vector<HQLOperand> x,
                           TARGET_TYPE t):
    HQLNode(LOGIC, AND, t)
{
    operands = x;
}


LogicAndNode::LogicAndNode(vector<shared_ptr<HQLNode> > x,
                           TARGET_TYPE t):
    HQLNode(LOGIC, AND, t)
{
    vector<shared_ptr<HQLNode> >::iterator it;
    for(it=x.begin(); it!=x.end(); ++it){
        push_operand(HQLOperand(*it));
    }
}



LogicAndNode::LogicAndNode(const LogicAndNode &n):
    HQLNode(n)
{
}

const LogicAndNode& LogicAndNode::operator=(const LogicAndNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

LogicAndNode::~LogicAndNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}

HQLNode* LogicAndNode::reduce()
{

    if(!validate()){
        return new ErrorNode(error_info);
    }

    HQLNode *ret = NULL;
    //1. expand and-operands

    vector<HQLNode*>::iterator it, bound;
    vector<HQLNode*> reduced_operands;
    //reduce all operands
    reduced_operands.resize(operands.size());
    std::transform(operands.begin(), operands.end(),
                   reduced_operands.begin(),
                   HQLNodeMemFunCaller<HQLOperand, HQLNode*, &HQLNode::reduce>());
    // taking all and-operands to front
    bound = std::partition(reduced_operands.begin(), reduced_operands.end(),
                           HQLNodeIsSubtype<HQLNode*, AND>());
    it = reduced_operands.begin();
    if(it == bound){ // no and-operands
        ret = new LogicAndNode(shared_ptr<HQLNode>(*it++),
                               shared_ptr<HQLNode>(*it++));
        while(it != reduced_operands.end()){
            ret->push_operand(HQLOperand(shared_ptr<HQLNode>(*it++)));
        }
    }else{ // has one or more and-operands, expand them
        ret = *it++;
        while(it != bound){
            ret->push_operand((*it)->get_operands());
            delete *it++;
        }
        while(it != reduced_operands.end()){
            ret->push_operand(HQLOperand(shared_ptr<HQLNode>(*it++)));
        }
    }

    //2. (and (not A) (not B)) -> not (or A B) keep only 0-or-1 not

    size_t not_cnt = std::count_if(ret->get_operands().begin(),
                                   ret->get_operands().end(),
                                   HQLNodeIsSubtype<HQLOperand, NOT>());
    if(not_cnt<=1){
        if(ret->validate()){
            return ret;
        }
        delete ret;
        return new ErrorNode(error_info + "[to fix 1]");
    }
    vector<HQLOperand> operands = ret->get_operands();
    delete ret;
    vector<HQLOperand>::iterator oit, obound;
    obound = std::partition(operands.begin(), operands.end(),
                            HQLNodeIsSubtype<HQLOperand, NOT>());
    oit = operands.begin();
    ret = new LogicOrNode((oit++)->as_node()->get_operand(0).as_node(),
                          (oit++)->as_node()->get_operand(0).as_node());
    while(oit!=obound){
        ret->push_operand((oit++)->as_node()->get_operand(0));
    }
    ret = new LogicNotNode(shared_ptr<HQLNode>(ret));
    if(oit!=operands.end()){
        ret = new LogicAndNode(shared_ptr<HQLNode>(ret), (oit++)->as_node());
        while(oit!=operands.end()){
            ret->push_operand(*oit++);
        }
    }

    if(ret->validate()){
        return ret;
    }
    delete ret;
    return new ErrorNode(error_info + "[to fix 2]");
}

HQLNode* LogicAndNode::copy()
{
    return new LogicAndNode(*this);
}

const string LogicAndNode::to_hql() const
{
    // more then 2 operands!
    vector<HQLOperand>::const_iterator it = operands.begin();
    vector<HQLOperand>::const_iterator last = --operands.end();
    string ret;
    while(it!=last){
        ret = ret + "(" + it->as_node()->to_hql() +") AND ";
        ++it;
    }
    ret = ret + "(" + it->as_node()->to_hql() +")";
    return ret;
}

const string LogicAndNode::cache_key(bool do_result_reduce) const
{
    if(do_result_reduce){
        vector<HQLOperand> tmp_operands = operands;
        std::sort(tmp_operands.begin(), tmp_operands.end(),
                  HQLNodeIsCacheKeyLess<HQLOperand>());
        vector<HQLOperand>::iterator vit = std::unique(tmp_operands.begin(), tmp_operands.end(),
                                                       HQLNodeIsCacheKeyEquals<HQLOperand>());
        tmp_operands.resize(vit - tmp_operands.begin());
        if(tmp_operands.size() > 1){
            return LogicAndNode(tmp_operands).cache_key(false);
        }else if(tmp_operands.size() == 1){
            return tmp_operands[0].as_node()->cache_key();
        }
    }


    vector<string> cache_keys;
    cache_keys.resize(operands.size());
    std::transform(operands.begin(), operands.end(), cache_keys.begin(),
                   std::bind2nd(HQLNodeConstMemFunCaller1<HQLOperand, const string, bool, &HQLNode::cache_key>(), false));
    std::sort(cache_keys.begin(), cache_keys.end());

    vector<string>::const_iterator it = cache_keys.begin();
    vector<string>::const_iterator last = --cache_keys.end();

    string ret="&[";
    while(it!=last){
        ret = ret  + *(it++) +",";
    }
    ret = ret + *it +"]";
    return ret;
}

const set<string> LogicAndNode::get_ctypes() const{
    set<string> ret = get_rtypes();
    if(ret.size()>0) return ret;
    return operands[0].as_node()->get_ctypes();
}

const set<string> LogicAndNode::get_rtypes() const{
    set<string> ret;
    for(size_t i=0; i<operands.size();i++){
        if(operands[i].get_type()!=HQLOperand::NODE) continue;
        set<string> ext = operands[i].as_node()->get_rtypes();
        ret.insert(ext.begin(), ext.end());
    }
    return ret;
}

ExtraMatchDataInfo LogicAndNode::get_xmdinfo() const{
    //TODO

    ExtraMatchDataInfo ret;

    size_t fk_cnt = std::count_if(operands.begin(), operands.end(),
                                  HQLNodeIsType<HQLOperand, SLFK>());
    size_t rl_cnt = std::count_if(operands.begin(), operands.end(),
                                  HQLNodeIsType<HQLOperand, RL>());

    if(fk_cnt<=0 && rl_cnt<=0){
        // 1. simple ANDs, need no Extra Data
    }else if(fk_cnt==1 and rl_cnt<=0){
        //2. 1-fk ANDs
        string host_type, guest_type, fk_name;

        for(size_t i=0; i<operands.size(); ++i){
            if(operands[i].as_node()->get_type()==HQLNode::SLFK){
                host_type = operands[i].as_node()->get_atype();
                guest_type = operands[i].as_node()->get_etype();
                fk_name = *(operands[i].as_node()->get_operand(1).as_str());
                break;
            }
        }

        for(size_t i=0; i<operands.size(); ++i){
            if(operands[i].as_node()->get_type()!=HQLNode::SLFK){
                string etype = operands[i].as_node()->get_etype();

                if(etype == host_type){
                    // need no Extra Data
                }else if(etype == guest_type){
                    ret.add_key(fk_name);
                }
            }
        }
    }else if(fk_cnt<=0 and rl_cnt==1){
        //2. 1-rel ANDs
    }else if(fk_cnt<=0 and rl_cnt>1){
        //3. n-rel ANDs
        /*
         * TODO:
         * Now, the parser only supports 2-Rel AND,
         * and the this match method (and this block) only supports 2-GAY-Rel AND
         */
        if(rl_cnt==2){
            ret += operands[0].as_node()->get_xmdinfo();
            ret += operands[1].as_node()->get_xmdinfo();
            ret.relation_info = 2;
        }
    }else{
        //4. errors node
    }

    return ret;
}

const map<uint64_t, set<string> > LogicAndNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, rel;
    if(m.attr<bool>("deleted")) return ret;
    size_t fk_cnt = std::count_if(operands.begin(), operands.end(),
                                  HQLNodeIsType<HQLOperand, SLFK>());
    size_t rl_cnt = std::count_if(operands.begin(), operands.end(),
                                  HQLNodeIsType<HQLOperand, RL>());

    if(fk_cnt<=0 && rl_cnt<=0){
        // 1. simple ANDs
        vector<map<uint64_t, set<string> > > mns;
        set<uint64_t> fns, fns_nomatch;
        mns.resize(operands.size());

        for(size_t i=0; i<operands.size(); ++i){
            mns[i] = operands[i].as_node()->match(m, getter);
            map<uint64_t, set<string> >::iterator it;
            for(it=mns[i].begin(); it!=mns[i].end(); ++it){
                fns.insert(it->first);
            }
        }

        for(set<uint64_t>::iterator fnit=fns.begin(); fnit!=fns.end(); ++fnit){
            for(size_t i=0; i<mns.size(); ++i){
                rel = mns[i];
                map<uint64_t, set<string> >::iterator it = rel.find(*fnit);
                if(it == rel.end() || it->second.size() == 0){
                    fns_nomatch.insert(*fnit);
                }
            }
        }

        for(set<uint64_t>::iterator fnit=fns_nomatch.begin(); fnit!=fns_nomatch.end(); ++fnit){
            fns.erase(*fnit);
        }

        for(set<uint64_t>::iterator fnit=fns.begin(); fnit!=fns.end(); ++fnit){
            uint64_t fn = *fnit;
            vector<vector<shared_ptr<HQLNode> > > fn_matches;
            vector<size_t> dimension, cycles;
            dimension.reserve(mns.size());
            cycles.reserve(mns.size());
            for(size_t i = mns.size(); i>0; --i){
                //set<string> matched_i = mns[i][fn];
                dimension[i-1] = mns[i-1][fn].size();
                if(i==mns.size()){
                    cycles[i-1] = 1;
                }else{
                    cycles[i-1] = dimension[i] * cycles[i];
                }
            }
            size_t matched_num = cycles[0]*dimension[0];
            fn_matches.reserve(matched_num);
            fn_matches.resize(matched_num);
            //std::for_each(fn_matches.begin(), fn_matches.end(),
            //              std::bind2nd(std::mem_fun_ref(&vector<shared_ptr<HQLNode> >::resize),
            //                           mns.size()));
            vector<vector<shared_ptr<HQLNode> > >::iterator nit;

            for(nit=fn_matches.begin(); nit!=fn_matches.end(); ++nit){
                nit->resize(mns.size());
            }

            for(size_t i = 0; i < mns.size(); ++i){
                set<string> matched_i = mns[i][fn];
                for(size_t k=0; k<matched_num/(cycles[i]*dimension[i]); k++){
                    size_t j=0;
                    for(set<string>::iterator it = matched_i.begin(); it!=matched_i.end(); ++it){
                        for(size_t t=0; t<cycles[i]; ++t){
                            size_t row = k*cycles[i]*dimension[i] + j*cycles[i] + t;
                            shared_ptr<HQLNode> p(new MatchedNode(*it));
                            fn_matches[row][i] = p;
                        }
                        j++;
                    }
                }
            }

            vector<vector<shared_ptr<HQLNode> > >::iterator it = fn_matches.begin();
            for(; it!=fn_matches.end(); ++it){
                ret[fn].insert(LogicAndNode(*it).cache_key(true));
            }
        }

    }else if(fk_cnt==1 and rl_cnt<=0){
        //2. 1-fk ANDs
        vector<vector<shared_ptr<HQLNode> > > mns;
        map<uint64_t, set<string> > tmp_matched;
        mns.resize(operands.size());
        string host_type, guest_type;
        uint64_t host_fn = m.fullname();
        uint64_t guest_fn = 0;
        JSONNode _n;
        Model guest(_n);

        for(size_t i=0; i<operands.size(); ++i){
            if(operands[i].as_node()->get_type()==HQLNode::SLFK){
                host_type = operands[i].as_node()->get_atype();
                guest_type = operands[i].as_node()->get_etype();

                string fk = *(operands[i].as_node()->get_operand(1).as_str());
                guest_fn = m.attr<uint64_t>(fk);
                if(TypeConfig::type_id(host_type)==0){
                    host_fn = FULLNAME_SET_CAT(host_fn);
                    guest_fn = FULLNAME_SET_CAT(guest_fn);
                }
                guest = ASTUtil::get_model(guest_fn, getter);
                tmp_matched = operands[i].as_node()->match(m, getter);
                if(tmp_matched[guest_fn].size()<=0)return ret;
                set<string>::iterator tmp_it=tmp_matched[guest_fn].begin();
                while(tmp_it != tmp_matched[guest_fn].end()){
                    mns[i].push_back(shared_ptr<HQLNode>(new MatchedNode(*tmp_it)));
                    ++tmp_it;
                }
            }
        }

        for(size_t i=0; i<operands.size(); ++i){
            if(operands[i].as_node()->get_type()!=HQLNode::SLFK){
                string etype = operands[i].as_node()->get_etype();

                if(etype == host_type){
                    tmp_matched = operands[i].as_node()->match(m, getter);
                    if(tmp_matched[host_fn].size()<=0){
                        return ret;
                    }
                    set<string>::iterator tmp_it=tmp_matched[host_fn].begin();
                    while(tmp_it != tmp_matched[host_fn].end()){
                        mns[i].push_back(shared_ptr<HQLNode>(new MatchedNode(*tmp_it)));
                        ++tmp_it;
                    }
                }else if(etype == guest_type){

                    tmp_matched = operands[i].as_node()->match(guest, getter);
                    if(tmp_matched[guest_fn].size()<=0){
                        return ret;
                    }
                    set<string>::iterator tmp_it=tmp_matched[guest_fn].begin();
                    while(tmp_it != tmp_matched[guest_fn].end()){
                        mns[i].push_back(shared_ptr<HQLNode>(new MatchedNode(*tmp_it)));
                        ++tmp_it;
                    }
                }
            }
        }

        vector<size_t> dimension, cycles;
        dimension.resize(mns.size());
        cycles.resize(mns.size());
        for(size_t i = mns.size(); i>0; --i){
            //set<string> matched_i = mns[i][fn];
            dimension[i-1] = mns[i-1].size();
            if(i==mns.size()){
                cycles[i-1] = 1;
            }else{
                cycles[i-1] = dimension[i] * cycles[i];
            }
        }

        size_t matched_num = cycles[0]*dimension[0];
        vector<vector<shared_ptr<HQLNode> > > fn_matches;
        fn_matches.resize(matched_num);

        vector<vector<shared_ptr<HQLNode> > >::iterator nit;

        for(nit=fn_matches.begin(); nit!=fn_matches.end(); ++nit){
            nit->resize(mns.size());
        }

        for(size_t i = 0; i < mns.size(); ++i){
            vector<shared_ptr<HQLNode> > matched_i = mns[i];
            for(size_t k=0; k<matched_num/(cycles[i]*dimension[i]); k++){
                for(size_t j =0; j<matched_i.size(); j++){
                    for(size_t t=0; t<cycles[i]; ++t){
                        size_t row = k*cycles[i]*dimension[i] + j*cycles[i] + t;
                        fn_matches[row][i] = matched_i[j];
                    }
                }
            }
        }

        vector<vector<shared_ptr<HQLNode> > >::iterator it = fn_matches.begin();
        for(; it!=fn_matches.end(); ++it){
            ret[guest_fn].insert(LogicAndNode(*it).cache_key(true));
        }

    }else if(fk_cnt<=0 and rl_cnt==1){
        //2. 1-rel ANDs
    }else if(fk_cnt<=0 and rl_cnt>1){
        //3. n-rel ANDs
        /*
         * TODO:
         * Now, the parser only supports 2-Rel AND,
         * and the this match method (and this block) only supports 2-GAY-Rel AND
         */
        if(rl_cnt==2){
            do{
                string rel0 = operands[0].as_node()->get_operand(0).as_node()->get_etype();
                string rel1 = operands[0].as_node()->get_operand(0).as_node()->get_etype();
                if(rel0!=rel1) break;
                if(TypeConfig::left_type(rel0)!=TypeConfig::right_type(rel0)) break;

                string reversed_rel_key = "R" + num2str(m.fullname()&0xFF) + "_" + num2str(m.right()) + "_" +num2str(m.left());
                vector<uint64_t> rr = ASTUtil::get_relations(reversed_rel_key, getter);
                if(rr.size()<1) break;
                Model mr = ASTUtil::get_model(rr[0], getter);

                map<uint64_t, set<string> > a0 = operands[0].as_node()->match(m, getter);
                map<uint64_t, set<string> > a1 = operands[1].as_node()->match(m, getter);

                map<uint64_t, set<string> > d0 = operands[0].as_node()->match(mr, getter);
                map<uint64_t, set<string> > d1 = operands[1].as_node()->match(mr, getter);

                map<uint64_t, set<string> >::iterator it;
                for(it=a0.begin(); it!=a0.end(); ++it){
                    shared_ptr<HQLNode> l(new MatchedNode(*it->second.begin()));
                    if(a1[it->first].size()>0){
                        shared_ptr<HQLNode> r(new MatchedNode(*a1[it->first].begin()));
                        ret[it->first].insert(LogicAndNode(l, r).cache_key());
                    }
                    if(d1[it->first].size()>0){
                        shared_ptr<HQLNode> r(new MatchedNode(*d1[it->first].begin()));
                        ret[it->first].insert(LogicAndNode(l, r).cache_key());
                    }
                }

                for(it=d0.begin(); it!=d0.end(); ++it){
                    shared_ptr<HQLNode> l(new MatchedNode(*it->second.begin()));
                    if(a1[it->first].size()>0){
                        shared_ptr<HQLNode> r(new MatchedNode(*a1[it->first].begin()));
                        ret[it->first].insert(LogicAndNode(l, r).cache_key());
                    }
                    if(d1[it->first].size()>0){
                        shared_ptr<HQLNode> r(new MatchedNode(*d1[it->first].begin()));
                        ret[it->first].insert(LogicAndNode(l, r).cache_key());
                    }
                }

            }while(0);
        }

    }else{
        //4. errors node
    }
    return ret;
}



bool LogicAndNode::validate() const{
    vector<HQLOperand>::const_iterator it = operands.begin();
    while(it != operands.end()){
        if(!(it->as_node()->validate())){
            error_info = "bad sub statement: " + it->as_node()->to_hql();
            return false;
        }
        ++it;
    }

    //TODO
    //1. find if has fk nodes
    //2. if has, ensure there etype and atype are same
    //3. ensure non-fk nodes' etype is etype and atype.
    //   if etype is atype, the node must be IMPLICIT
    //size_t fk_cnt = std::count_if(operands.begin(), operands.end(),
    //                              HQLNodeIsType<HQLOperand, SLFK>());
    vector<string> fk_types = has_semantic_fk();
    if(fk_types.size()!=2){ // no slfk nodes
        it = operands.begin();
        etype = (it++)->as_node()->get_etype();
        while(it != operands.end()){
            if((it++)->as_node()->get_etype() != etype){
                error_info = "1.logic operands have different etypes";
                return false;
            }
        }
        return true;
    }

    //TODO: bug on nested node!

    etype = fk_types[0];
    atype = fk_types[1];

    vector<HQLOperand> tmp_operands = operands;
    vector<HQLOperand>::iterator oit, obound;
    obound = std::partition(tmp_operands.begin(), tmp_operands.end(),
                            HQLNodeIsType<HQLOperand, SLFK>());
    oit = tmp_operands.begin();

    while(oit!=obound){
        if(oit->as_node()->get_etype()!=etype ||
           oit->as_node()->get_atype()!=atype){
            error_info = "2.logic operands have different etypes or atypes";
            return false;
        }
        oit++;
    }
    if(oit!=operands.end()){
        if(oit->as_node()->get_etype()==etype){
            // this node is ok
        }else if(oit->as_node()->get_etype()==atype){
            if(oit->as_node()->get_target()==EXPLICIT){
                error_info = "3.logic operands have different etypes or atypes";
                return false;
            }
            // this node is ok
        }
    }
    return true;
}

bool LogicAndNode::has_semantic_each() const{
    return std::find_if(operands.begin(), operands.end(),
                        HQLNodeConstMemFunCaller<HQLOperand, bool, &HQLNode::has_semantic_each>())
        != operands.end();
}

pair<string, uint64_t> LogicAndNode::time_in() const
{
    vector<HQLOperand>::const_iterator it;
    for(it=operands.begin(); it!=operands.end(); ++it){
        pair<string, uint64_t> ret = it->as_node()->time_in();
        if(ret.first.size()>0 )return ret;
    }
    return std::make_pair("", 0UL);
}

vector<string> LogicAndNode::has_semantic_fk() const
{
    vector<HQLOperand>::const_iterator it;
    for(it=operands.begin(); it!=operands.end(); ++it){
        vector<string> ret = it->as_node()->has_semantic_fk();
        if(ret.size()==2)return ret;
    }
    return vector<string>();
}


/* LogicOrNode */

LogicOrNode::LogicOrNode(shared_ptr<HQLNode> l,
                         shared_ptr<HQLNode> r,
                         TARGET_TYPE t):
    HQLNode(LOGIC, OR, t)
{
    push_operand(HQLOperand(l));
    push_operand(HQLOperand(r));
}


LogicOrNode::LogicOrNode(const LogicOrNode &n):
    HQLNode(n)
{
}

const LogicOrNode& LogicOrNode::operator=(const LogicOrNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

LogicOrNode::~LogicOrNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}


HQLNode* LogicOrNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }

    HQLNode *ret = NULL;
    //1. expand or-operands

    vector<HQLNode*>::iterator it, bound;
    vector<HQLNode*> reduced_operands;
    //reduce all operands
    reduced_operands.resize(operands.size());
    std::transform(operands.begin(), operands.end(),
                   reduced_operands.begin(),
                   HQLNodeMemFunCaller<HQLOperand, HQLNode*, &HQLNode::reduce>());
    // taking all or-operands to front
    bound = std::partition(reduced_operands.begin(), reduced_operands.end(),
                           HQLNodeIsSubtype<HQLNode*, OR>());
    it = reduced_operands.begin();
    if(it == bound){ // no or-operands
        ret = new LogicOrNode(shared_ptr<HQLNode>(*it++),
                              shared_ptr<HQLNode>(*it++));
        while(it != reduced_operands.end()){
            ret->push_operand(HQLOperand(shared_ptr<HQLNode>(*it++)));
        }
    }else{ // has one or more or-operands, expand them
        ret = *it++;
        while(it != bound){
            ret->push_operand((*it)->get_operands());
            delete *it++;
        }
        while(it != reduced_operands.end()){
            ret->push_operand(HQLOperand(shared_ptr<HQLNode>(*it++)));
        }
    }

    //2. (or (not A) (not B) ... ) -> not (and A B ...) , keep only 0-or-1 not
    size_t not_cnt = std::count_if(ret->get_operands().begin(),
                                   ret->get_operands().end(),
                                   HQLNodeIsSubtype<HQLOperand, NOT>());
    if(not_cnt<=1){
        return ret;
    }
    vector<HQLOperand> operands = ret->get_operands();
    delete ret;
    vector<HQLOperand>::iterator oit, obound;
    obound = std::partition(operands.begin(), operands.end(),
                            HQLNodeIsSubtype<HQLOperand, NOT>());
    oit = operands.begin();
    ret = new LogicAndNode((oit++)->as_node()->get_operand(0).as_node(),
                           (oit++)->as_node()->get_operand(0).as_node());
    while(oit!=obound){
        ret->push_operand((oit++)->as_node()->get_operand(0));
    }
    ret = new LogicNotNode(shared_ptr<HQLNode>(ret));
    if(oit!=operands.end()){
        ret = new LogicOrNode(shared_ptr<HQLNode>(ret), (oit++)->as_node());
        while(oit!=operands.end()){
            ret->push_operand(*oit++);
        }
    }
    return ret;
}

HQLNode* LogicOrNode::copy()
{
    return new LogicOrNode(*this);
}


const string LogicOrNode::to_hql() const
{
    // more then 2 operands!
    vector<HQLOperand>::const_iterator it = operands.begin();
    vector<HQLOperand>::const_iterator last = --operands.end();
    string ret;
    while(it!=last){
        ret = ret + "(" + it->as_node()->to_hql() +") OR ";
        ++it;
    }
    ret = ret + "(" + it->as_node()->to_hql() +")";
    return ret;
}

const string LogicOrNode::cache_key(bool do_result_reduce) const
{
    vector<string> cache_keys;
    cache_keys.resize(operands.size());
    std::transform(operands.begin(), operands.end(), cache_keys.begin(),
                   std::bind2nd(HQLNodeConstMemFunCaller1<HQLOperand, const string, bool, &HQLNode::cache_key>(), false));
    std::sort(cache_keys.begin(), cache_keys.end());

    vector<string>::const_iterator it = cache_keys.begin();
    vector<string>::const_iterator last = --cache_keys.end();

    string ret="|[";
    while(it!=last){
        ret = ret  + *(it++) +",";
    }
    ret = ret + *it +"]";
    return ret;
}

const set<string> LogicOrNode::get_ctypes() const{
    set<string> ret = get_rtypes();
    if(ret.size()>0) return ret;
    return operands[0].as_node()->get_ctypes();
}

const set<string> LogicOrNode::get_rtypes() const{
    set<string> ret;
    for(size_t i=0; i<operands.size();i++){
        if(operands[0].get_type()!=HQLOperand::NODE) continue;
        set<string> ext = operands[0].as_node()->get_rtypes();
        ret.insert(ext.begin(), ext.end());
    }
    return ret;
}

ExtraMatchDataInfo LogicOrNode::get_xmdinfo() const{
    ExtraMatchDataInfo ret;

    for(size_t i=0; i<operands.size(); ++i){
        ret += operands[i].as_node()->get_xmdinfo();
    }
    return ret;
}

const map<uint64_t, set<string> > LogicOrNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, rel;
    if(m.attr<bool>("deleted")) return ret;
    vector<map<uint64_t, set<string> > > rels;
    rels.resize(operands.size());

    for(size_t i=0; i<operands.size(); ++i){
        rels[i] = operands[i].as_node()->match(m, getter);
    }
    for(size_t i=0; i<rels.size(); ++i){
        rel = rels[i];
        map<uint64_t, set<string> >::iterator it;
        for(it = rel.begin(); it!=rel.end(); ++it){
            uint64_t fn = it->first;
            if(rel[fn].size()>0){
                ret[fn].insert(this->cache_key());
            }
        }
    }
    return ret;
}


bool LogicOrNode::validate() const{
    vector<HQLOperand>::const_iterator it = operands.begin();
    while(it != operands.end()){
        if(!(it->as_node()->validate())){
            error_info = "bad sub statement: " + it->as_node()->to_hql();
            return false;
        }
        if(it->as_node()->get_type() == HQLNode::SLFK){
            error_info = "SLFK in LogicOrNode";
            return false;
        }
        if(it->as_node()->get_type() == HQLNode::RL){
            error_info = "RL in LogicOrNode";
            return false;
        }
        ++it;
    }

    it = operands.begin();
    etype = (it++)->as_node()->get_etype();
    while(it != operands.end()){
        if((it++)->as_node()->get_etype() != etype){
            error_info = "logic operands have different etypes";
            return false;
        }
    }

    return true;
}

bool LogicOrNode::has_semantic_each() const{
    return std::find_if(operands.begin(), operands.end(),
                        HQLNodeConstMemFunCaller<HQLOperand, bool, &HQLNode::has_semantic_each>())
        != operands.end();
}

pair<string, uint64_t> LogicOrNode::time_in() const
{
    vector<HQLOperand>::const_iterator it;
    for(it=operands.begin(); it!=operands.end(); ++it){
        pair<string, uint64_t> ret = it->as_node()->time_in();
        if(ret.first.size()>0 )return ret;
    }
    return std::make_pair("", 0UL);
}

vector<string> LogicOrNode::has_semantic_fk() const
{
    vector<HQLOperand>::const_iterator it;
    for(it=operands.begin(); it!=operands.end(); ++it){
        vector<string> ret = it->as_node()->has_semantic_fk();
        if(ret.size()==2)return ret;
    }
    return vector<string>();
}

/* LogicNotNode */

LogicNotNode::LogicNotNode(shared_ptr<HQLNode> o):
    HQLNode(LOGIC, NOT, o->get_target())
{
    etype = o->get_etype();
    push_operand(HQLOperand(o));
}


LogicNotNode::LogicNotNode(const LogicNotNode &n):
    HQLNode(n)
{
}

const LogicNotNode& LogicNotNode::operator=(const LogicNotNode& n)
{
    if(this == &n)return *this;
    HQLNode::operator=(n);
    return *this;
}

LogicNotNode::~LogicNotNode()
{
#ifdef DEBUG
    std::cout<< "<DEBUG> HQLNode DTOR: "  << this->to_hql() << std::endl;
#endif
}


HQLNode* LogicNotNode::reduce()
{
    if(!validate()){
        return new ErrorNode(error_info);
    }

    shared_ptr<HQLNode> obj = operands[0].as_node();
    HQLNode *robj = obj->reduce();
    if(robj->get_type() == LOGIC and robj->get_subtype() == NOT){
        HQLNode *ret = robj->get_operand(0).as_node()->copy();
        delete robj;
        return ret;
    }else{
        LogicNotNode *ret = new LogicNotNode(shared_ptr<HQLNode>(robj));
        return ret;
    }
}

HQLNode* LogicNotNode::copy()
{
    return new LogicNotNode(*this);
}

const string LogicNotNode::to_hql() const
{
    return "NOT (" + operands[0].as_node()->to_hql() + ")";
}

const string LogicNotNode::cache_key(bool do_result_reduce) const
{
    return "![" + operands[0].as_node()->cache_key() +"]";
}

const set<string> LogicNotNode::get_ctypes() const{
    return operands[0].as_node()->get_ctypes();
}

const set<string> LogicNotNode::get_rtypes() const{
    return operands[0].as_node()->get_rtypes();
}

ExtraMatchDataInfo LogicNotNode::get_xmdinfo() const{
    return operands[0].as_node()->get_xmdinfo();
}


const map<uint64_t, set<string> > LogicNotNode::match(const Model &m, ModelGetter *getter)
{
    map<uint64_t, set<string> > ret, rel;
    if(m.attr<bool>("deleted")) return ret;
    uint64_t fn = m.fullname();
    rel = operands[0].as_node()->match(m, getter);
    if(rel[fn].size()<=0){
        ret[fn].insert(this->cache_key());
    }
    return ret;
}

bool LogicNotNode::validate() const
{
    if(!operands[0].as_node()->validate()){
        error_info = "bad sub statement: " + operands[0].as_node()->to_hql();
        return false;
    }
    if(operands[0].as_node()->has_semantic_each()){
        error_info = "bad not statement: sub statement has semantic-EACH";
        return false;
    }
    etype = operands[0].as_node()->get_etype();
    return true;
}

bool LogicNotNode::has_semantic_each() const{
    return false;
}

pair<string, uint64_t> LogicNotNode::time_in() const
{
    return operands[0].as_node()->time_in();
}

vector<string> LogicNotNode::has_semantic_fk() const
{
    return operands[0].as_node()->has_semantic_fk();
}
