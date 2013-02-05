/*
 * python module HQL :
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

#undef _POSIX_C_SOURCE
#undef _XOPEN_SOURCE
#include "Python.h"

using std::ifstream;

static PyObject *HQLError;


/*
 * Util Functions
 */

static void _hql_dict2json(PyObject *dict, JSONNode &n);
static void _hql_flatobj2json(const string key, PyObject* value, JSONNode &n) {
    if(value == Py_None){
        JSONNode sub_node(JSON_NULL);
        sub_node.set_name(key);
        n.push_back(sub_node);
    }else if(PyBool_Check(value)){
        n.push_back(JSONNode(key, value == Py_True));
    }else if(PyInt_Check(value)){
        n.push_back(JSONNode(key, PyInt_AS_LONG(value)));
    }else if(PyLong_Check(value)){
        n.push_back(JSONNode(key, PyLong_AsLong(value)));
    }else if(PyFloat_Check(value)){
        n.push_back(JSONNode(key, PyFloat_AsDouble(value)));
    }else if(PyString_Check(value)){
        n.push_back(JSONNode(key, PyString_AS_STRING(value)));
    }else if(PyUnicode_Check(value)){
        n.push_back(JSONNode(key, PyUnicode_AS_DATA(value)));
    }else if(PyTuple_Check(value) || PyList_Check(value)){
        JSONNode sub_node(JSON_ARRAY);
        sub_node.set_name(key);
        _hql_dict2json(value, sub_node);
        n.push_back(sub_node);
    }else if(PyDict_Check(value)){
        JSONNode sub_node(JSON_NODE);
        sub_node.set_name(key);
        _hql_dict2json(value, sub_node);
        n.push_back(sub_node);
    }
}

static void _hql_dict2json(PyObject *dict, JSONNode &n) {
    PyObject *key, *value;
    Py_ssize_t pos = 0;

    if(PyTuple_Check(dict)){
        for(int i=0; i<PyTuple_Size(dict); i++){
            value = PyTuple_GetItem(dict, i);
            _hql_flatobj2json("", value, n);
        }
    }else if(PyList_Check(dict)){
        for(int i=0; i<PyList_Size(dict); i++){
            value = PyList_GetItem(dict, i);
            _hql_flatobj2json("", value, n);
        }
    }else if(PyDict_Check(dict)){
        while (PyDict_Next(dict, &pos, &key, &value)) {
            const string skey = PyString_AS_STRING(key);
            _hql_flatobj2json(skey, value, n);
        }
    }
}

static JSONNode hql_dict2json(PyObject *dict) {
    JSONNode ret;

    if(PyDict_Check(dict)){
        _hql_dict2json(dict, ret);
    }

#ifdef DEBUG
    std::cerr<< "<DEBUG>  LuaTable2JSON JSON=: "  << ret.write() << std::endl;
#endif

    return ret;
}


template<void(_F)(HQLNode*, uint8_t, bool), bool _xpc>
static PyObject *_hql_ctl_troller(PyObject *self, PyObject *args, PyObject *kw){
    uint8_t ns = 0;
    int n = -1;
    int xpc = _xpc ? 1 : 0;
    char *hql_str;
    PyObject *result;
    static const char *kwlist[] = {"hql", "ns", "xpc", NULL};

    if (!PyArg_ParseTupleAndKeywords(
            args, kw, "s|ii", const_cast<char **>(kwlist), &hql_str, &n, &xpc)) {
        result = Py_False;
        Py_INCREF(result);
        return result;
    }
    HQLNode *node = ASTUtil::parser_hql(hql_str);
    if(node){
        ns = n<0 ? TrollersHolder::cur_ns_num : (uint8_t)n;
        _F(node, ns, xpc!=0);
        delete node;
        result = Py_True;
    }else{
        result = Py_False;
    }
    Py_INCREF(result);
    return result;
}

template<bool _xpc>
static PyObject *_hql_clear_trollers(PyObject *self, PyObject *args){
    uint8_t ns = 0;
    int xpc = _xpc ? 1 : 0;
    int n = -1;
    PyObject *result;
    if (!PyArg_ParseTuple(args, "|ii", &n, &xpc)){
        result = Py_False;
        Py_INCREF(result);
        return result;
    }
    ns = n<0 ? TrollersHolder::cur_ns_num : (uint8_t)n;
    TrollersHolder::clear_trollers(ns, xpc!=0);
    result = Py_True;
    Py_INCREF(result);
    return result;
}


extern "C" {

    /*
     * API Functions
     */

    static PyObject *HQL_setup_config(PyObject *self, PyObject *args) {
        const char *config;
        PyObject *result;
        if (!PyArg_ParseTuple(args, "s", &config)){
            result = Py_False;
            Py_INCREF(result);
            return result;
        }
        TypeConfig::setup_from_json(config);
        result = Py_True;
        Py_INCREF(result);
        return result;
    }


    static PyObject *HQL_use_namespace(PyObject *self, PyObject *args){
        int n = -1;
        PyObject *result;
        if (!PyArg_ParseTuple(args, "|i", &n)){
            result = Py_False;
            Py_INCREF(result);
            return result;
        }
        if(n<0){
            return Py_BuildValue("i", TrollersHolder::cur_ns_num);
        }
        n = TrollersHolder::use_namespace((uint8_t)n);
        return Py_BuildValue("i", n);
    }

    static PyObject *HQL_register_troller(PyObject *self, PyObject *args, PyObject *kw){
        return _hql_ctl_troller<TrollersHolder::register_troller, false>(
            self, args, kw
        );
    }

    static PyObject *HQL_xregister_troller(PyObject *self, PyObject *args, PyObject *kw){
        return _hql_ctl_troller<TrollersHolder::register_troller, true>(
            self, args, kw
        );
    }

    static PyObject *HQL_unregister_troller(PyObject *self, PyObject *args, PyObject *kw){
        return _hql_ctl_troller<TrollersHolder::unregister_troller, false>(
            self, args, kw
        );
    }

    static PyObject *HQL_xunregister_troller(PyObject *self, PyObject *args, PyObject *kw){
        return _hql_ctl_troller<TrollersHolder::unregister_troller, true>(
            self, args, kw
        );
    }

    static PyObject *HQL_clear_trollers(PyObject *self, PyObject *args){
        return _hql_clear_trollers<false>(self, args);
    }

    static PyObject *HQL_xclear_trollers(PyObject *self, PyObject *args){
        return _hql_clear_trollers<true>(self, args);
    }

    static PyObject *HQL_hql2hql(PyObject *self, PyObject *args){
        char *hql_str;
        PyObject *result;
        if (!PyArg_ParseTuple(args, "s", &hql_str)) {
            result = Py_False;
            Py_INCREF(result);
            return result;
        }
        HQLNode *node = ASTUtil::parser_hql(hql_str);
        if(node){
            string hql = node->to_hql();
            delete node;
            return Py_BuildValue("s", hql.c_str());
        }else{
            result = Py_False;
            Py_INCREF(result);
        }
        return result;
    }


    static PyObject *HQL_hql2cachekey(PyObject *self, PyObject *args){
        int do_result_reduce = 1;
        char *hql_str;
        PyObject *result;

        if (!PyArg_ParseTuple(args, "s|i", &hql_str, &do_result_reduce)) {
            result = Py_False;
            Py_INCREF(result);
            return result;
        }
        HQLNode *node = ASTUtil::parser_hql(hql_str);
        if(node){
            uint8_t type_id = TypeConfig::type_id(node->get_etype());
            string hql = "LIST[" + num2str((uint8_t)type_id) + "]" + node->cache_key(do_result_reduce!=0);
            delete node;
            return Py_BuildValue("s", hql.c_str());
        }else{
            result = Py_False;
        }
        Py_INCREF(result);
        return result;
    }

    static PyObject *HQL_all_trollers(PyObject *self, PyObject *args){
        int i = -1;
        PyObject* result = PyDict_New();
        PyArg_ParseTuple(args, "|i", &i);
        uint8_t ns = i<0 ? TrollersHolder::cur_ns_num : (uint8_t)i;

        const map<string, list<HQLNode*> >& trs = TrollersHolder::get_trollers(ns);
        map<string, list<HQLNode*> >::const_iterator it;
        for(it=trs.begin(); it != trs.end(); it++){
            PyObject *key = Py_BuildValue("s", it->first.c_str());
            PyObject *value = PyList_New(it->second.size());
            list<HQLNode*>::const_iterator sit = it->second.begin();
            int i=0;
            for(; sit != it->second.end(); sit++){
                PyList_SetItem(value, i, Py_BuildValue("s",(*sit)->to_hql().c_str()));
                i++;
            }
            PyDict_SetItem(result, key, value);
        }
        return result;
    }

    static PyObject *HQL_hql_info(PyObject *self, PyObject *args){
        char *hql_str;
        PyObject *result = PyDict_New();
        if (!PyArg_ParseTuple(args, "s", &hql_str)) {
            return result;
        }
        string m = hql_str;

        if(m.size()<8) return result; //LIST[n]L[num,[HQL]]
        if(m.substr(0,5)=="LIST["){ // a cache-key

            do{// TIME_IN info
                string::size_type tip = m.find(",TIME_IN,");
                if(tip == string::npos) break;
                string part_l = m.substr(0, tip);
                string part_r = m.substr(tip+9);
                tip = part_l.find_last_of('.');
                if(tip == string::npos) break;
                part_l = part_l.substr(tip+1);

                PyDict_SetItem(
                    result,
                    Py_BuildValue("s", "time_in_key"), //key
                    Py_BuildValue("s", part_l.c_str()) //value
                );

                PyDict_SetItem(
                    result,
                    Py_BuildValue("s", "time_in_seconds"), //key
                    Py_BuildValue("l", atol(part_r.c_str())) //value
                );
            }while(0);

            string::size_type tmp = m.find_first_of(']');
            if(tmp==string::npos) goto end;
            m = m.substr(tmp+1);
            if(m[0]=='O' && m[1]=='['){
                m = m.substr(2);
                string::size_type p = m.find_first_of(",");
                PyDict_SetItem(
                    result,
                    Py_BuildValue("s", "order_key"), //key
                    Py_BuildValue("s", m.substr(0,p).c_str()) //value
                );

                m = m.substr(p+1);
                PyDict_SetItem(
                    result,
                    Py_BuildValue("s", "order_asc"), //key
                    (m[0]=='A' ? Py_True : Py_False) //value
                );
            }
            tmp = m.find_last_of(']');
            m = m.substr(tmp+1);
            if(m[0]=='L'){
                PyDict_SetItem(
                    result,
                    Py_BuildValue("s", "limit"), //key
                    Py_BuildValue("l", atol(m.c_str()+1)) //value
                );
            }
        }else{ // a hql
            HQLNode *n = ASTUtil::parser_hql(m);
            HQLNode *old_n = n;
            if(n){
                pair<string, uint64_t> ret = n->time_in();
                if(ret.first.size()>0){
                    PyDict_SetItem(
                        result,
                        Py_BuildValue("s", "time_in_key"), //key
                        Py_BuildValue("s", ret.first.c_str()) //value
                    );

                    PyDict_SetItem(
                        result,
                        Py_BuildValue("s", "time_in_seconds"), //key
                        Py_BuildValue("l", ret.second) //value
                    );
                }
                if(n->get_type() == HQLNode::MISC && n->get_subtype()== HQLNode::LIMIT){
                    PyDict_SetItem(
                        result,
                        Py_BuildValue("s", "limit"), //key
                        Py_BuildValue("l", atol(n->get_operand(1).as_str()->c_str())) //value
                    );
                    printf("!!!!limit = %s \n",n->get_operand(1).as_str()->c_str());

                    n = n->get_operand(0).as_node().get();
                }
                if(n->get_type() == HQLNode::MISC && n->get_subtype()== HQLNode::ORDER_BY){
                    PyDict_SetItem(
                        result,
                        Py_BuildValue("s", "order_key"), //key
                        Py_BuildValue("s", n->get_operand(1).as_str()->c_str()) //value
                    );

                    PyDict_SetItem(
                        result,
                        Py_BuildValue("s", "order_asc"), //key
                        (n->get_operand(1).as_num()==0 ? Py_True : Py_False) //value
                    );
                }
                delete old_n;
            }
        }
    end:
        return result;
    }

    static PyObject *HQL_extmd_info(PyObject *self, PyObject *args){
        int i = -1;
        PyObject* result = PyDict_New();
        PyArg_ParseTuple(args, "|i", &i);
        uint8_t ns = i<0 ? TrollersHolder::cur_ns_num : (uint8_t)i;

        const map<string, ExtraMatchDataInfo>& extmd = TrollersHolder::get_extmd_info(ns);
        map<string, ExtraMatchDataInfo>::const_iterator it;

        for(it =  extmd.begin(); it != extmd.end(); it++){
            PyObject *key = Py_BuildValue("s", it->first.c_str());
            PyObject *value = PyDict_New();
            PyObject *key_0 = Py_BuildValue("s", "keys");
            PyObject *value_0 = PyList_New(it->second.keys.size());
            set<string>::iterator sit = it->second.keys.begin();
            int i=0;
            for(; sit != it->second.keys.end(); sit++){
                PyList_SetItem(
                    value_0, i,
                    Py_BuildValue("s", sit->c_str())
                );
                i++;
            }
            PyDict_SetItem(value, key_0, value_0);
            PyDict_SetItem(
                value,
                Py_BuildValue("s", "relation_info"),
                Py_BuildValue("l", it->second.relation_info)
            );

            PyDict_SetItem(result, key, value);
        }
        return result;
    }



    static PyObject *HQL_xmatch(PyObject *self, PyObject *args){

        int i = -1;
        PyObject *obj, *extdb;
        PyObject *result = PyDict_New();
        if(!PyArg_ParseTuple(args, "OO|i", &obj, &extdb, &i)){
            return result;
        }
        uint8_t ns = i<0 ? TrollersHolder::cur_ns_num : (uint8_t)i;

        map<uint64_t, set<string> > ret;
        JSONNode extmd = hql_dict2json(extdb);
        JSONModelGetter getter(&extmd);

        JSONNode n = hql_dict2json(obj);
        ret = TrollersHolder::xmatch(n, &getter, ns);

        map<uint64_t, set<string> >::iterator it;
        for(it=ret.begin(); it != ret.end(); it++){
            PyObject *fn = Py_BuildValue("l", it->first);
            PyObject *va = PyList_New(it->second.size());
            set<string>::iterator sit = it->second.begin();
            int i=0;
            for(; sit != it->second.end(); sit++){
                PyList_SetItem(va, i, Py_BuildValue("s",sit->c_str()));
                i++;
            }
            PyDict_SetItem(result, fn, va);
        }
        return result;
    }

    static PyMethodDef HQLMethods[] = {
        {"setup_config", HQL_setup_config, METH_VARARGS, "setup HQL config."},
        {"use_namespace", HQL_use_namespace, METH_VARARGS, "change/get current namespace."},
        {"register_troller", (PyCFunction)HQL_register_troller, METH_VARARGS|METH_KEYWORDS, "register hql."},
        {"register_hql", (PyCFunction)HQL_register_troller, METH_VARARGS|METH_KEYWORDS, "register hql."}, //alias
        {"xregister_troller", (PyCFunction)HQL_xregister_troller, METH_VARARGS|METH_KEYWORDS, "register hql(xpc version)."},
        {"xregister_hql", (PyCFunction)HQL_xregister_troller, METH_VARARGS|METH_KEYWORDS, "register hql(xpc version)."}, //alias
        {"unregister_troller", (PyCFunction)HQL_unregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql."},
        {"unregister_hql", (PyCFunction)HQL_unregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql."}, //alias
        {"xunregister_troller", (PyCFunction)HQL_xunregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql(xpc version)."},
        {"xunregister_hql", (PyCFunction)HQL_xunregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql(xpc version)."}, //alias
        {"clear_trollers", (PyCFunction)HQL_clear_trollers, METH_VARARGS, "clear all hql in namespace."},
        {"clear_hql", (PyCFunction)HQL_clear_trollers, METH_VARARGS, "clear all hql in namespace."}, //alias
        {"xclear_trollers", (PyCFunction)HQL_xclear_trollers, METH_VARARGS, "clear all hql in namespace (xpc version)."},
        {"xclear_hql", (PyCFunction)HQL_xclear_trollers, METH_VARARGS, "clear all hql in namespace (xpc version)."}, //alias
        {"trollers", (PyCFunction)HQL_all_trollers, METH_VARARGS, "list all hql in namespace."},
        {"all_hql", (PyCFunction)HQL_all_trollers, METH_VARARGS, "list all hql in namespace."}, //alias
        {"hql2hql", (PyCFunction)HQL_hql2hql, METH_VARARGS, "format given hql."},
        {"format_hql", (PyCFunction)HQL_hql2hql, METH_VARARGS, "format given hql."}, //alias
        {"hql2cachekey", (PyCFunction)HQL_hql2cachekey, METH_VARARGS, "get cachekey of hql."}, //alias
        {"hql_info", (PyCFunction)HQL_hql_info, METH_VARARGS, "get info of hql."},
        {"extmd_info", (PyCFunction)HQL_extmd_info, METH_VARARGS, "get ext match data info of namespace."},
        {"xmatch", (PyCFunction)HQL_xmatch, METH_VARARGS, "XMATCH!"},
        {NULL, NULL, 0, NULL}        /* Sentinel */
    };

    PyMODINIT_FUNC
    inithql(void)
    {
        PyObject *m;

        m = Py_InitModule("hql", HQLMethods);
        if (m == NULL)
            return;

        char errclz[] = "hql.error";
        HQLError = PyErr_NewException(errclz, NULL, NULL);
        Py_INCREF(HQLError);
        PyModule_AddObject(m, "error", HQLError);
    }
}
