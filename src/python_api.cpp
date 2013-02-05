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

//TODO
extern "C" {

    /*
     * API Functions
     */

    static PyObject *HQL_setup_config(PyObject *self, PyObject *args) {
        const char *config;
        if (!PyArg_ParseTuple(args, "s", &config)){
            //Py_INCREF(Py_False);
            return Py_False;
        }
        TypeConfig::setup_from_json(config);
        //Py_INCREF(Py_True);
        return Py_True;
    }

    static PyMethodDef HQLMethods[] = {
        {"setup_config",  HQL_setup_config, METH_VARARGS, "setup HQL config."},
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
