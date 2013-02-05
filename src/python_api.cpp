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

    static PyMethodDef HQLMethods[] = {
        {"setup_config", HQL_setup_config, METH_VARARGS, "setup HQL config."},
        {"use_namespace", HQL_use_namespace, METH_VARARGS, "change/get current namespace."},
        {"register_troller", (PyCFunction)HQL_register_troller, METH_VARARGS|METH_KEYWORDS, "register hql."},
        {"xregister_troller", (PyCFunction)HQL_xregister_troller, METH_VARARGS|METH_KEYWORDS, "register hql(xpc version)."},
        {"unregister_troller", (PyCFunction)HQL_unregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql."},
        {"xunregister_troller", (PyCFunction)HQL_xunregister_troller, METH_VARARGS|METH_KEYWORDS, "unregister hql(xpc version)."},
        {"clear_trollers", (PyCFunction)HQL_clear_trollers, METH_VARARGS, "clear all hql in namespace."},
        {"xclear_trollers", (PyCFunction)HQL_xclear_trollers, METH_VARARGS, "clear all hql in namespace (xpc version)."},
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
