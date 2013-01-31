/* -*- c++ -*-
 *
 * file: trollers.hpp
 * author: KDr2
 *
 */

#ifndef _TROLLERS_HPP
#define _TROLLERS_HPP

#define NDEBUG
#include "../vendor/libjson/libjson.h"
#undef  NDEBUG


#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>

#include "ast.hpp"
#include "model.hpp"
#include "ast_util.hpp"

using std::map;
using std::set;
using std::list;
using std::string;
using std::vector;

class TrollersNamespace{
public:
    void register_troller(HQLNode*);
    void unregister_troller(HQLNode*);
    void recal_extmd_info();
    map<uint64_t, set<string> > xmatch(const string&, ModelGetter*);
    map<uint64_t, set<string> > xmatch(const JSONNode&, ModelGetter*);

    void clear();

    map<string, ExtraMatchDataInfo> extmd_info;
    map<string, list<HQLNode*> > trollers;
};



class TrollersHolder{
public:
    static uint8_t set_max_ns_num(uint8_t);
    static uint8_t use_namespace(uint8_t);

    static void register_troller(HQLNode*, bool xpc=false);
    static void register_troller(HQLNode*, uint8_t ns, bool xpc=false);

    static void unregister_troller(HQLNode*, bool xpc=false);
    static void unregister_troller(HQLNode*, uint8_t ns, bool xpc=false);

    static void clear_trollers(bool xpc=false);
    static void clear_trollers(uint8_t ns, bool xpc=false);

    static map<uint64_t, set<string> > xmatch(const string&, ModelGetter*);
    static map<uint64_t, set<string> > xmatch(const string&, ModelGetter*, uint8_t ns);

    static map<uint64_t, set<string> > xmatch(const JSONNode&, ModelGetter*);
    static map<uint64_t, set<string> > xmatch(const JSONNode&, ModelGetter*, uint8_t ns);

    static const map<string, ExtraMatchDataInfo>& get_extmd_info();
    static const map<string, ExtraMatchDataInfo>& get_extmd_info(uint8_t ns);

    static const map<string, list<HQLNode*> >& get_trollers();
    static const map<string, list<HQLNode*> >& get_trollers(uint8_t ns);

    static vector<TrollersNamespace*> namespaces;
    static uint8_t max_ns_num;
    static uint8_t cur_ns_num;
    static bool mp_mode;
};


#endif /* _TROLLERS_HPP */
