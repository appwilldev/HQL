# -*- coding: utf-8 -*-
# author : KDr2

import hql

class HQL(object):

    def __init__(self, str_hql):
        formated_hql = hql.format_hql(str_hql)
        if not formated_hql: raise Exception("Bad HQL")
        self._hql = formated_hql
        self._info = None

    def __str__(self):
        return self._hql

    def register(self, ns=None, xpc=False):
        fn = hql.xregister_hql if xpc else hql.register_hql
        return fn(self._hql, ns) if ns!=None else fn(self._hql)

    def unregister(self, ns=None, xpc=False):
        fn = hql.xunregister_hql if xpc else hql.unregister_hql
        return fn(self._hql, ns) if ns!=None else fn(self._hql)

    def cachekey(self):
        return hql.hql2cachekey(self._hql)

    def info(self):
        if not self._info:
            self._info = hql.hql_info(self._hql)
        #endif
        return self._info


    @classmethod
    def setup_config(clz, json):
        return hql.setup_config(json)

    @classmethod
    def all_hql(clz, ns=None):
        return hql.all_hql(ns) if ns!=None else hql.all_hql()

    @classmethod
    def clear_namespace(clz, ns=None, xpc=False):
        fn = hql.xclear_trollers if xpc else hql.clear_trollers
        return fn(ns) if ns!=None else fn()

    @classmethod
    def namespace(clz, ns=None):
        return hql.use_namespace(ns) if ns!=None else hql.usenamespace()

    @classmethod
    def extmd_info(clz, ns=None):
        return hql.extmd_info(ns) if ns!=None else hql.extmd_info()

    @classmethod
    def xmatch(clz, m, extdata=None, ns=None):
        if not extdata: extdata = {}
        if ns is None:
            return hql.xmatch(m, extdata)
        return hql.xmatch(m, extdata, ns)
