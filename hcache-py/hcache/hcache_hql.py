# -*- coding: utf-8 -*-
# author : KDr2

import hql
from hcache_model import Model

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

    def fmt(self, fmt, *args):
        hql_str = self._hql
        components = hql_str.split("EACH")
        components_len = len(components)-1
        if components_len!=len(fmt) or components_len!=len(args) or len(fmt)!=len(args):
            return None
        #endif

        formated_args = []
        idx = 0
        for arg in args:
            if isinstance(arg,Model): arg = arg.fullname
            fmt_key = fmt[idx:idx+1]
            if fmt_key=='s':
                arg = unicode(arg).replace("\"", "\\\"")
                formated_args.append('"' + arg + '"')
            elif fmt_key=='d':
                formated_args.append(str(arg))
            elif fmt_key=='b':
                if arg==True or (isinstance(arg, (str,unicode)) and arg.lower()=='true'):
                    arg = "true"
                else:
                    arg = "false"
                 #end
                formated_args.append(arg)
            elif fmt_key=='f':
                formated_args.append("#" + str(arg))
            else:
                formated_args.append(str(arg))
            #endif
                idx = idx + 1
        #endfor

        idx = 0
        for i in range(components_len, 0, -1):
            components.insert(i, formated_args[components_len-idx-1])
            idx = idx + 1
        #endfor
        return HQL("".join(components))


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

    @classmethod
    def keyinfo(self, key):
        return  hql.hql_info(key)
