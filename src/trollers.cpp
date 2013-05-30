/* -*- c++ -*-
 *
 * file: trollers.cpp
 * author: KDr2
 *
 */

#include <algorithm>

#include "model.hpp"
#include "trollers.hpp"
#include "type_config.hpp"
#include "ast_util.hpp"
#include "cmn_util.hpp"
#include "hql_xpc.hpp"

void TrollersNamespace::register_troller(HQLNode *n){
    set<string> ctypes = n->get_ctypes();
    set<string>::iterator it = ctypes.begin();
    for(; it!= ctypes.end(); it++){
        for(list<HQLNode*>::iterator nit = trollers[*it].begin();
            nit != trollers[*it].end(); ){
            if((*nit)->cache_key(false) == n->cache_key(false)){
                // already added, via the only way: call this method
                return;
            }else{
                ++nit;
            }
        }
        trollers[*it].push_back(n->copy());
        extmd_info[*it] += n->get_xmdinfo();
    }
}

void TrollersNamespace::unregister_troller(HQLNode *n){
    set<string> ctypes = n->get_ctypes();
    set<string>::iterator it = ctypes.begin();
    for(; it!= ctypes.end(); it++){
        for(list<HQLNode*>::iterator nit = trollers[*it].begin();
            nit != trollers[*it].end(); ){
            if((*nit)->cache_key(false) == n->cache_key(false)){
                delete *nit;
                nit = trollers[*it].erase(nit);
                break;
            }else{
                ++nit;
            }
        }
    }
    recal_extmd_info();
}

void TrollersNamespace::recal_extmd_info(){
    extmd_info.clear();
    map<string, list<HQLNode*> >::iterator it = trollers.begin();
    for(; it!= trollers.end(); it++){
        list<HQLNode*>::iterator hit = it->second.begin();
        for(; hit != it->second.end(); ++hit){
            extmd_info[it->first] += (*hit)->get_xmdinfo();
        }
    }
}

map<uint64_t, set<string> > TrollersNamespace::xmatch(const string &m, ModelGetter *getter){
    map<uint64_t, set<string> > ret;
    JSONNode n;
    try{
        n = libjson::parse(m);
        if(n.type()!=JSON_NODE) goto end;
        ret = xmatch(n, getter);
    }catch(std::invalid_argument e){
        //PASS
    }
 end:
    return ret;
}


map<uint64_t, set<string> > TrollersNamespace::xmatch(const JSONNode &n, ModelGetter *getter){
    map<uint64_t, set<string> > ret, tmp;

    if(n.type()!=JSON_NODE){
        goto end;
    }else{
        //uint64_t fn = n.at("fullname").as_int();
        Model m(n);
        if(m.attr<bool>("deleted")) goto end;

        list<HQLNode*> t = trollers[m.type()];
        list<HQLNode*>::iterator it = t.begin();
        for(; it != t.end(); it++){
            tmp = (*it)->match(m, getter);
            map<uint64_t, set<string> >::iterator rit = tmp.begin();
            for(; rit!=tmp.end(); rit++){
                uint64_t fn = rit->first;
                uint8_t type_id = FULLNAME_IS_CAT(fn) ? 0 : (fn & 0xFF);
                fn = FULLNAME_RM_CAT(fn);
                set<string>::iterator kit = rit->second.begin();
                for(; kit!=rit->second.end(); ++kit){
                    ret[fn].insert("LIST[" + num2str(type_id) + "]" + *kit);
                }
            }
        }
    }
 end:
    return ret;
}

void TrollersNamespace::clear(){
    for(map<string, list<HQLNode*> >::iterator it = trollers.begin();
        it != trollers.end();
        ++it){
        list<HQLNode*> *tl = &(it->second);
        for(list<HQLNode*>::iterator lit = tl->begin(); lit != tl->end(); ++lit){
            delete *lit;
        }
    }
    trollers.clear();
    extmd_info.clear();
}

uint8_t TrollersHolder::max_ns_num = 0;
uint8_t TrollersHolder::cur_ns_num = 0;
vector<TrollersNamespace*> TrollersHolder::namespaces;
bool TrollersHolder::mp_mode = false;

uint8_t TrollersHolder::set_max_ns_num(uint8_t m){
    if(max_ns_num > 0) return max_ns_num;
    for(uint8_t i = 0; i < m; i++){
        namespaces.push_back(new TrollersNamespace());
    }
    max_ns_num = m;
    return m;
}

uint8_t TrollersHolder::use_namespace(uint8_t c){
    if(c < max_ns_num) cur_ns_num = c;
    return cur_ns_num;
}

void TrollersHolder::register_troller(HQLNode *n, bool xpc){
    register_troller(n, cur_ns_num, xpc);
}

void TrollersHolder::register_troller(HQLNode *n, uint8_t ns, bool xpc){
    if(ns>=namespaces.size()) return;
    namespaces[ns]->register_troller(n);
    if(mp_mode && xpc){
        int64_t id = HQLXPController::get_id();
        HQLXPController::add_delta(
            HQLXPCDelta::ADD, id, 1, ns, n->to_hql().c_str());
    }
}

void TrollersHolder::unregister_troller(HQLNode *n, bool xpc){
    unregister_troller(n, cur_ns_num, xpc);
}

void TrollersHolder::unregister_troller(HQLNode *n, uint8_t ns, bool xpc){
    if(ns>=namespaces.size()) return;
    namespaces[ns]->unregister_troller(n);
    if(mp_mode && xpc){
        int64_t id = HQLXPController::get_id();
        HQLXPController::add_delta(
            HQLXPCDelta::DEL, id, 1, ns, n->to_hql().c_str());
    }
}

void TrollersHolder::clear_trollers(bool xpc){
    clear_trollers(cur_ns_num, xpc);
}

void TrollersHolder::clear_trollers(uint8_t ns, bool xpc){
    if(ns>=namespaces.size()) return;
    namespaces[ns]->clear();
    if(mp_mode && xpc){
        int64_t id = HQLXPController::get_id();
        HQLXPController::add_delta(
            HQLXPCDelta::CLR, id, 1, ns, NULL);
    }
}

map<uint64_t, set<string> > TrollersHolder::xmatch(const string &m, ModelGetter *getter){
    if(mp_mode){
        HQLXPController::check_delta();
    }
    return xmatch(m, getter, cur_ns_num);
}

map<uint64_t, set<string> > TrollersHolder::xmatch(const string &m, ModelGetter *getter, uint8_t ns){
    if(mp_mode){
        HQLXPController::check_delta();
    }
    if(ns>= namespaces.size()) return map<uint64_t, set<string> >();
    return namespaces[ns]->xmatch(m, getter);
}


map<uint64_t, set<string> > TrollersHolder::xmatch(const JSONNode &n, ModelGetter *getter){
    if(mp_mode){
        HQLXPController::check_delta();
    }
    return xmatch(n, getter, cur_ns_num);
}

map<uint64_t, set<string> > TrollersHolder::xmatch(const JSONNode &n, ModelGetter *getter, uint8_t ns){
    if(mp_mode){
        HQLXPController::check_delta();
    }
    if(ns>=namespaces.size()) return map<uint64_t, set<string> >();
    return namespaces[ns]->xmatch(n, getter);
}

static const map<string, ExtraMatchDataInfo> empty_extmd_info;
static const map<string, list<HQLNode*> > empty_trollers;

const map<string, ExtraMatchDataInfo>& TrollersHolder::get_extmd_info()
{
    return get_extmd_info(cur_ns_num);
}

const map<string, ExtraMatchDataInfo>& TrollersHolder::get_extmd_info(uint8_t ns)
{
    if(ns>=namespaces.size()) return empty_extmd_info;
    return namespaces[ns]->extmd_info;
}

const map<string, list<HQLNode*> >& TrollersHolder::get_trollers()
{
    return get_trollers(cur_ns_num);
}

const map<string, list<HQLNode*> >& TrollersHolder::get_trollers(uint8_t ns)
{
    if(ns>=namespaces.size()) return empty_trollers;
    return namespaces[ns]->trollers;
}
