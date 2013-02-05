/*
 * lua module HQL :
 *   Haddit Query Language
 *
 * author : KDr2
 *
 */

#include "ast.hpp"
#include "ast_util.hpp"
#include "cmd.hpp"
#include "parser.hpp"
#include "type_config.hpp"
#include "trollers.hpp"
#include "cmn_util.hpp"

#include <iostream>
#include <fstream>

#define LUA_LIB
#include "lua.hpp"

using std::ifstream;


/*
 * Util Functions
 */


static void _hql_table2json(lua_State *L, int index, JSONNode &n) {
    lua_pushvalue(L, index);
    // stack now contains: -1 => table
    lua_pushnil(L);
    // stack now contains: -1 => nil; -2 => table
    while (lua_next(L, -2)){
        // stack now contains: -1 => value; -2 => key; -3 => table
        // copy the key so that lua_tostring does not modify the original
        lua_pushvalue(L, -2);
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        const char *key = lua_tostring(L, -1);
        if(lua_isboolean(L, -2)){
            n.push_back(JSONNode(key, !!lua_toboolean(L, -2)));
        }else if(lua_isnumber(L,-2)){
            n.push_back(JSONNode(key, lua_tonumber(L, -2)));
        }else if(lua_isstring(L, -2)){
            const char *value = lua_tostring(L, -2);
            n.push_back(JSONNode(key, value));
        }else if(lua_istable(L, -2)){
            JSONNode sub_node;
            sub_node.set_name(key);
            _hql_table2json(L, -2, sub_node);
            n.push_back(sub_node);
        }
        // pop value + copy of key, leaving original key
        lua_pop(L, 2);
        // stack now contains: -1 => key; -2 => table
    }
    // stack now contains: -1 => table (when lua_next returns 0 it pops the key
    // but does not push anything.)
    // Pop table
    lua_pop(L, 1);
}

static JSONNode hql_table2json(lua_State *L, int index) {
    JSONNode ret;
    if(lua_istable(L, index)){
        _hql_table2json(L, index, ret);
    }

#ifdef DEBUG
    std::cerr<< "<DEBUG>  LuaTable2JSON JSON=: "  << ret.write() << std::endl;
#endif

    return ret;
}


template<bool xpc>
static int _hql_register_troller(lua_State *L) {
    uint8_t ns = 0;
    if(lua_gettop(L) == 2){
        ns = static_cast<uint8_t>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }else{
        ns = TrollersHolder::cur_ns_num;
    }

    if(!lua_isstring(L, -1)){
        lua_pop(L, 1);
        lua_pushboolean(L, 0);
        return 1;
    }
    HQLNode *n = ASTUtil::parser_hql(lua_tostring(L, -1));
    lua_pop(L, 1);
    if(n){
        TrollersHolder::register_troller(n, ns, xpc);
        delete n;
        lua_pushboolean(L, 1);
    }else{
        lua_pushboolean(L, 0);
    }
    return 1;
}

template<bool xpc>
static int _hql_unregister_troller(lua_State *L) {
    uint8_t ns = 0;
    if(lua_gettop(L) == 2){
        ns = static_cast<uint8_t>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }else{
        ns = TrollersHolder::cur_ns_num;
    }

    if(!lua_isstring(L, -1)){
        lua_pop(L, 1);
        lua_pushboolean(L, 0);
        return 1;
    }

    HQLNode *n = ASTUtil::parser_hql(lua_tostring(L, -1));
    lua_pop(L, 1);
    if(n){
        TrollersHolder::unregister_troller(n, ns, xpc);
        delete n;
        lua_pushboolean(L, 1);
    }else{
        lua_pushboolean(L, 0);
    }
    return 1;
}

template<bool xpc>
static int _hql_clear_trollers(lua_State *L) {
    uint8_t ns = 0;
    if(lua_gettop(L) == 1){
        ns = static_cast<uint8_t>(lua_tonumber(L, -1));
        lua_pop(L, 1);
    }else{
        ns = TrollersHolder::cur_ns_num;
    }

    TrollersHolder::clear_trollers(ns, xpc);
    lua_pushnumber(L, ns);
    return 1;
}


extern "C" {

    /*
     * API Functions
     */

    static int HQL_setup_config(lua_State *L) {
        string json = string(lua_tostring(L, -1));
        lua_pop(L, 1);
        TypeConfig::setup_from_json(json);
        lua_pushboolean(L, 1);
        return 1;
    }

    static int HQL_use_namespace(lua_State *L) {
        uint8_t n = 0;
        if(lua_gettop(L) == 0){
            lua_pushnumber(L, TrollersHolder::cur_ns_num);
            return 1;
        }
        if(lua_isnumber(L, -1)){
            n = static_cast<uint8_t>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }
        n = TrollersHolder::use_namespace(n);
        lua_pushnumber(L, n);
        return 1;
    }

    static int HQL_register_troller(lua_State *L) {
        return _hql_register_troller<false>(L);
    }

    static int HQL_xregister_troller(lua_State *L) {
        return _hql_register_troller<true>(L);
    }

    static int HQL_unregister_troller(lua_State *L) {
        return _hql_unregister_troller<false>(L);
    }

    static int HQL_xunregister_troller(lua_State *L) {
        return _hql_unregister_troller<true>(L);
    }

    static int HQL_clear_trollers(lua_State *L) {
        return _hql_clear_trollers<false>(L);
    }

    static int HQL_xclear_trollers(lua_State *L) {
        return _hql_clear_trollers<true>(L);
    }

    static int HQL_hql2hql(lua_State *L) {
        if(!lua_isstring(L, -1)){
            lua_pop(L, 1);
            return 0;
        }
        HQLNode *n = ASTUtil::parser_hql(lua_tostring(L, -1));
        lua_pop(L, 1);
        if(n){
            lua_pushboolean(L, 1);
            string lua = n->to_hql();
            delete n;
            lua_pushstring(L, lua.c_str());
        }else{
            return 0;
        }
        return 2;
    }


    static int HQL_hql2cachekey(lua_State *L) {
        bool do_result_reduce = true;
        if(lua_gettop(L) == 2){
            do_result_reduce = lua_toboolean(L, -1);
            lua_pop(L, 1);
        }

        if(!lua_isstring(L, -1)){
            lua_pop(L, 1);
            return 0;
        }
        HQLNode *n = ASTUtil::parser_hql(lua_tostring(L, -1));
        lua_pop(L, 1);
        if(n){
            uint8_t type_id = TypeConfig::type_id(n->get_etype());
            string lua = "LIST[" + num2str((uint8_t)type_id) + "]" + n->cache_key(do_result_reduce);
            delete n;
            lua_pushstring(L, lua.c_str());
        }else{
            return 0;
        }
        return 1;
    }

    static int HQL_all_trollers(lua_State *L){
        uint8_t ns = 0;
        if(lua_gettop(L) == 1){
            ns = static_cast<uint8_t>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }else{
            ns =TrollersHolder::cur_ns_num;
        }

        const map<string, list<HQLNode*> >& trs = TrollersHolder::get_trollers(ns);

        lua_newtable(L);
        map<string, list<HQLNode*> >::const_iterator it;
        for(it=trs.begin(); it != trs.end(); it++){
            lua_pushstring(L, it->first.c_str());
            lua_newtable(L);
            list<HQLNode*>::const_iterator sit = it->second.begin();
            int i=0;
            for(; sit != it->second.end(); sit++){
                lua_pushnumber(L, i+1);
                lua_pushstring(L, (*sit)->to_hql().c_str());
                lua_settable(L, -3);
                i++;
            }
            lua_settable(L, -3);
        }
        return 1;
    }


    static int HQL_hql_info(lua_State *L){
        string m;
        if(lua_isstring(L, -1)){
            m = lua_tostring(L, -1);
            lua_pop(L, 1);
        }
        lua_newtable(L);

        if(m.size()<8) return 1; //LIST[n]L[num,[HQL]]
        if(m.substr(0,5)=="LIST["){ // a cache-key

            do{// TIME_IN info
                string::size_type tip = m.find(",TIME_IN,");
                if(tip == string::npos) break;
                string part_l = m.substr(0, tip);
                string part_r = m.substr(tip+9);
                tip = part_l.find_last_of('.');
                if(tip == string::npos) break;
                part_l = part_l.substr(tip+1);

                lua_pushstring(L, "time_in_key");
                lua_pushstring(L, part_l.c_str());
                lua_settable(L, -3);

                lua_pushstring(L, "time_in_seconds");
                lua_pushnumber(L, atol(part_r.c_str()));
                lua_settable(L, -3);
            }while(0);

            string::size_type tmp = m.find_first_of(']');
            if(tmp==string::npos) goto end;
            m = m.substr(tmp+1);
            if(m[0]=='O' && m[1]=='['){
                m = m.substr(2);
                string::size_type p = m.find_first_of(",");
                lua_pushstring(L, "order_key");
                lua_pushstring(L, m.substr(0,p).c_str());
                lua_settable(L, -3);

                m = m.substr(p+1);
                lua_pushstring(L, "order_asc");
                lua_pushboolean(L, m[0]=='A');
                lua_settable(L, -3);
            }
            tmp = m.find_last_of(']');
            m = m.substr(tmp+1);
            if(m[0]=='L'){
                lua_pushstring(L, "limit");
                lua_pushnumber(L, atol(m.c_str()+1));
                lua_settable(L, -3);
            }
        }else{ // a hql
            HQLNode *n = ASTUtil::parser_hql(m);
            if(n){
                pair<string, uint64_t> ret = n->time_in();
                if(ret.first.size()>0){
                    lua_pushstring(L, "time_in_key");
                    lua_pushstring(L, ret.first.c_str());
                    lua_settable(L, -3);

                    lua_pushstring(L, "time_in_seconds");
                    lua_pushnumber(L, ret.second);
                    lua_settable(L, -3);
                }
                if(n->get_type() == HQLNode::MISC && n->get_subtype()== HQLNode::LIMIT){
                    lua_pushstring(L, "limit");
                    lua_pushnumber(L, atol(n->get_operand(1).as_str()->c_str()));
                    lua_settable(L, -3);
                    n = n->get_operand(0).as_node().get();
                }
                if(n->get_type() == HQLNode::MISC && n->get_subtype()== HQLNode::ORDER_BY){
                    lua_pushstring(L, "order_key");
                    lua_pushstring(L, n->get_operand(1).as_str()->c_str());
                    lua_settable(L, -3);

                    lua_pushstring(L, "order_asc");
                    lua_pushboolean(L, n->get_operand(1).as_num()==0);
                    lua_settable(L, -3);
                }
                delete n;
            }
        }
    end:
        return 1;
    }

    static int HQL_extmd_info(lua_State *L){
        uint8_t ns = 0;
        if(lua_gettop(L) == 1){
            ns = static_cast<uint8_t>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }else{
            ns = TrollersHolder::cur_ns_num;
        }

        const map<string, ExtraMatchDataInfo>& extmd = TrollersHolder::get_extmd_info(ns);

        map<string, ExtraMatchDataInfo>::const_iterator it;
        lua_newtable(L);

        for(it =  extmd.begin(); it != extmd.end(); it++){
            lua_pushstring(L, it->first.c_str());
            lua_newtable(L);

            lua_pushstring(L, "keys");
            lua_newtable(L);
            set<string>::iterator sit = it->second.keys.begin();
            int i=0;
            for(; sit != it->second.keys.end(); sit++){
                lua_pushnumber(L, i+1);
                lua_pushstring(L, sit->c_str());
                lua_settable(L, -3);
                i++;
            }
            lua_settable(L, -3);

            lua_pushstring(L, "relation_info");
            lua_pushnumber(L, it->second.relation_info);
            lua_settable(L, -3);

            lua_settable(L, -3);
        }
        return 1;
    }


    static int HQL_xmatch(lua_State *L){

        uint8_t ns = 0;
        if(lua_gettop(L) == 3){
            ns = static_cast<uint8_t>(lua_tonumber(L, -1));
            lua_pop(L, 1);
        }else{
            ns = TrollersHolder::cur_ns_num;
        }

        map<uint64_t, set<string> > ret;
        JSONNode extmd = hql_table2json(L, -1);
        JSONModelGetter getter(&extmd);

        if(lua_isstring(L, -2)){
            string m = lua_tostring(L, -2);
            ret = TrollersHolder::xmatch(m, &getter, ns);
        }else if(lua_istable(L, -2)){
            JSONNode n = hql_table2json(L, -2);
            ret = TrollersHolder::xmatch(n, &getter, ns);
        }
        lua_pop(L, 2);

        lua_newtable(L);
        map<uint64_t, set<string> >::iterator it;
        for(it=ret.begin(); it != ret.end(); it++){
            lua_pushnumber(L, it->first);
            lua_newtable(L);
            set<string>::iterator sit = it->second.begin();
            int i=0;
            for(; sit != it->second.end(); sit++){
                lua_pushnumber(L, i+1);
                lua_pushstring(L, sit->c_str());
                lua_settable(L, -3);
                i++;
            }
            lua_settable(L, -3);
        }
        return 1;
    }

    static const luaL_Reg hql[] = {
        {"setup_config", HQL_setup_config},
        {"use_namespace", HQL_use_namespace}, // non-arg for current namespace
        {"register_troller", HQL_register_troller}, // ns
        {"xregister_troller", HQL_xregister_troller}, // ns
        {"unregister_troller", HQL_unregister_troller}, // ns
        {"xunregister_troller", HQL_xunregister_troller}, // ns
        {"clear_trollers", HQL_clear_trollers}, // ns
        {"xclear_trollers", HQL_xclear_trollers}, // ns
        {"trollers", HQL_all_trollers}, // ns
        {"hql2hql", HQL_hql2hql},
        {"format_hql", HQL_hql2hql},
        {"hql2cachekey", HQL_hql2cachekey},
        {"hql_info", HQL_hql_info},
        {"extmd_info", HQL_extmd_info}, //ns
        {"xmatch", HQL_xmatch}, //ns
        {NULL, NULL}
    };


    LUALIB_API int luaopen_hql (lua_State *L) {
        luaL_register(L, "hql", hql);
        return 1;
    }
}
