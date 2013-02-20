# -*- coding: utf-8 -*-
# author : KDr2

import re
import sys
import simplejson as json

HCacheConfig={}


class TypeInfo(object):

    #model_name -> type_id
    __TYPES__ = {}
    #type_id -> model_name
    __RTYPES__ = {}
    #relation_name -> tuple(left_type,right_type)
    __RELATION_TYPES__ = {}

    __TYPE_CLASSES__ = {}

    @classmethod
    def reset_registers(cls):
        old_types = cls.__TYPES__
        cls.__TYPES__ = {}
        old_rtypes = cls.__RTYPES__
        cls.__RTYPES__ = {}
        old_rel_types = cls.__RELATION_TYPES__
        cls.__RELATION_TYPES__ = {}
        cls.__TYPE_CLASSES__ = {}
        return (old_types,old_rtypes,old_rel_types)

    @classmethod
    def register_class(cls, name, clz):
        cls.__TYPE_CLASSES__[name.lower()] = clz

    @classmethod
    def register_entity(cls, name, type_id):
        tid = type_id&0xff
        cls.__TYPES__[name] = tid
        cls.__RTYPES__[tid] = name

    @classmethod
    def register_relation(cls, rel_name, type_id, left_t, right_t):
        rid = (0x80+type_id)&0xff
        cls.__TYPES__[rel_name] = rid
        cls.__RTYPES__[rid] = rel_name
        lt = cls.model_type_id(left_t)
        rt = cls.model_type_id(right_t)
        cls.__RELATION_TYPES__[rel_name] = (lt,rt)

    @classmethod
    def type_name(cls, tid):
        type_name = cls.__RTYPES__[tid]
        return type_name

    @classmethod
    def model_type_id(cls, type_name):
        #TYPE ID RANGE
        #0x00-0x80 : entity
        #0x81-0xEF : relation type-id
        #0xF0-0xFF : reserved type-id

        if cls.__TYPES__.has_key(type_name):
            return cls.__TYPES__[type_name]
        raise Exception("Unkown Model Type")

    @classmethod
    def relation_element_type(cls, type_name):
        if isinstance(type_name, int):
            type_name = cls.__RTYPES__[type_name]
        if cls.__TYPES__.has_key(type_name) and cls.__RELATION_TYPES__.has_key(type_name):
            return cls.__RELATION_TYPES__[type_name]
        raise Exception("Unkown Relation Type")

def setup_types(dct):
    types=dct.get("type",{})
    e_types=types.get("entity") or {}
    for k,v in e_types.iteritems():
        TypeInfo.register_entity(k.lower(),v)
    r_types=types.get("relation") or {}
    for k,v in r_types.iteritems():
        TypeInfo.register_relation(k.lower(),v[0],v[1].lower(),v[2].lower())


def setup(dct):
    global HCacheConfig
    HCacheConfig = dct
    setup_types(dct)
    import hcache_hql
    hcache_hql.HQL.setup_config(json.dumps(dct))
    import hcache_capi
    hcache_capi.setup(dct)
