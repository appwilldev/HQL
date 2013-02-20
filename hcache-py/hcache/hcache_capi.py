#-*- coding:utf-8 -*-
# author : KDr2


# Cache API for HCache

import os

from hcache_model import *

import hcache_config as config
import hcache_local as localcache

#########
#scheme
#########
WALL=1<<0
WONE=1<<1
RALL=1<<2
RONE=1<<3
#shortcut
ONE=WONE|RONE
ALL=WALL|RONE

#R/W
KL_READ  = 1<<0
KL_WRITE = 1<<1
KL_RW    = KL_READ|KL_WRITE
KV_READ  = 1<<2
KV_WRITE = 1<<3
KV_RW    = KV_READ|KV_WRITE

#order by
ORDER_ASC=1
ORDER_DESC=0

def force_key(key):
    if isinstance(key,str) \
            or isinstance(key,unicode) \
            or isinstance(key,int) \
            or isinstance(key,long):
        return key
    if isinstance(key, Model):
        return key.fullname()
    if isinstance(key, list):
        return key
    return str(key)

def backend_key(key):
    if isinstance(key,(str,unicode)):
        if key.isdigit():
            #dback=dynamic_backend_key(key)
            #if dback:return dback
            fn=int(key)&0xFF
            return config.TypeInfo.type_name(fn) + "-value"
        elif key.startswith("LIST"):
            #LIST[typeid]xxx
            fn=int(key[5:key.find(']')])
            return "%s-list" % config.TypeInfo.type_name(fn)
        elif key.startswith("R"):
            fn=int(key[1:key.find("_")])&0xFF
            return config.TypeInfo.type_name(fn) + "-rel"
        else:
            return "default"
    if isinstance(key,(int,long)):
        #dback=dynamic_backend_key(key)
        #if dback:return dback
        return config.TypeInfo.type_name(key&0xFF) + "-value"
    if isinstance(key, Model):
        #dback=dynamic_backend_key(key.fullname)
        #if dback:return dback
        return key.type_name + "-value"
    if isinstance(key, ListDescriptor):
        return "%s-list" % config.TypeInfo.type_name(key._element_type)
    return "default"


class CacheConfig(object):

    __CACHE_BCAKENDS__={}
    __BACKENDS_MAP__={}

    @classmethod
    def get_backends(cls,key):
        if len(cls.__CACHE_BCAKENDS__)==1:
            return cls.__CACHE_BCAKENDS__.values()
        key=backend_key(key)

        if isinstance(key,dict):
            # dynamic banckend from TT
            backends=[cls.__CACHE_BCAKENDS__[x] for x in key["backends"]]
            key["backends"]=backends
            return key

        keys=[]
        if cls.__BACKENDS_MAP__.has_key(key):
            keys=cls.__BACKENDS_MAP__[key]
        elif key.endswith("-value"):
            if cls.__BACKENDS_MAP__.has_key("default-value"):
                keys=cls.__BACKENDS_MAP__["default-value"]
            else:
                key=key[:len(key)-6]
                if cls.__BACKENDS_MAP__.has_key(key):
                    keys=cls.__BACKENDS_MAP__[key]
        elif key.endswith("-list"):
            if cls.__BACKENDS_MAP__.has_key("default-list"):
                keys=cls.__BACKENDS_MAP__["default-list"]
            else:
                key=key[:len(key)-5]
                if cls.__BACKENDS_MAP__.has_key(key):
                    keys=cls.__BACKENDS_MAP__[key]
        elif key.endswith("-attr"):
            if cls.__BACKENDS_MAP__.has_key("default-attr"):
                keys=cls.__BACKENDS_MAP__["default-attr"]
            else:
                key=key[:len(key)-5]
                if cls.__BACKENDS_MAP__.has_key(key):
                    keys=cls.__BACKENDS_MAP__[key]
        elif key.endswith("-rel"):
            if cls.__BACKENDS_MAP__.has_key("default-rel"):
                keys=cls.__BACKENDS_MAP__["default-rel"]
            else:
                key=key[:len(key)-4]
                if cls.__BACKENDS_MAP__.has_key(key):
                    keys=cls.__BACKENDS_MAP__[key]
        if not keys:
            if cls.__BACKENDS_MAP__.has_key("default"):
                keys=cls.__BACKENDS_MAP__["default"]
        backends=[]
        for k in keys:
            if cls.__CACHE_BCAKENDS__.has_key(k):
                backends.append(cls.__CACHE_BCAKENDS__[k])
        return backends


    @classmethod
    def add_backend(cls,name,backend):
        cls.__CACHE_BCAKENDS__[name]=backend

    @classmethod
    def map(cls,typ,backends):
        cls.__BACKENDS_MAP__[typ]=backends


def setup(dct):
    cache_backends=dct['cache_backend']
    cache_map=dct['cache_map']
    #1. setup backends

    #1.0 setup localcache
    import hcache_local as localcache
    lc_backends=cache_backends.get('localcache',{}) or {}
    for k,v in lc_backends.iteritems():
        try:
            rbe=localcache.CacheLocal(v)
            CacheConfig.add_backend(k,rbe)
            print "Add LocalCache-Backend:[%s=%s] ok!" % (k,v)
        except:
            print "Add LocalCache-Backend:[%s=%s] error!" % (k,v)

    #1.1 setup redis
    import hcache_redis
    redis_backends=cache_backends.get('redis',{}) or {}
    for k,v in redis_backends.iteritems():
        try:
            rbe=hcache_redis.CacheRedis(v)
            CacheConfig.add_backend(k,rbe)
            print "Add Redis-Backend:[%s=%s] ok!" % (k,v)
        except:
            print "Add Redis-Backend:[%s=%s] error!" % (k,v)

    #1.2 setup em-redis if need:
    emredis_config=cache_backends.get('emredis',None)
    if emredis_config and os.path.exists(emredis_config):
        print "********************************"
        print "***** Adding Embeded Redis *****"
        print "********************************"
        import cache_emrds
        for index in range(128):
            emrds=cache_emrds.CacheEmbededRedis(emredis_config,db=index)
            CacheConfig.add_backend("emredis_%d" % index,emrds)

    #1.3 setup nessdb
    nessdb_backends=cache_backends.get('nessdb',{}) or {}
    if nessdb_backends:import cache_nessdb
    for k,v in nessdb_backends.iteritems():
        try:
            rbe=cache_nessdb.CacheNessDB(v)
            CacheConfig.add_backend(k,rbe)
            print "Add NessDB-Backend:[%s=%s] ok!" % (k,v)
        except:
            print "Add NessDB-Backend:[%s=%s] error!" % (k,v)
    #2. map backend
    for k,v in cache_map.iteritems():
        CacheConfig.map(k,v)

class cache_api(object):

    def __init__(self, scheme, rw=0):
        self.scheme=scheme
        self.rw=rw


    def __call__(self,fun):
        fname=fun.func_name
        def inner(cls,key,*args,**kwargs):
            backends=CacheConfig.get_backends(key)
            if isinstance(backends,dict):
                # dynamic backends from TT
                extra_kwrags=backends
                backends=backends['backends']
                del extra_kwrags['backends']
                if self.rw==KV_READ:
                    kwargs.update(extra_kwrags.get("read_options",{}))
                elif self.rw==KV_WRITE:
                    kwargs.update(extra_kwrags.get("write_options",{}))

            key=force_key(key)
            ret=[]
            if self.scheme&WALL==WALL:
                if isinstance(key,(Entity,Relation)):
                    key._dirty_keys=[]
                try:
                    ret = [getattr(be,fname)(key,*args,**kwargs) for be in backends]
                except Exception,e:
                    ret="ERROR:%s" % str(e)
                    return False #write error!
            elif self.scheme&WONE==WONE:
                for be in backends:
                    r0=getattr(be,fname)(key,*args,**kwargs)
                    if r0:
                        ret.append(r0)
                        break
            if not ret:return None
            if self.scheme&RALL==RALL:
                return ret
            elif self.scheme&RONE==RONE:
                return ret[0]
        return inner

#--------API------

class KeyListCache(object):


    @classmethod
    @cache_api(ONE)
    def slist_range(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def slist_add(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def slist_update(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def slist_del_val(*args,**kwargs):pass

    @classmethod
    @cache_api(ONE)
    def slist_len(*args,**kwargs):pass

    @classmethod
    @cache_api(ONE)
    def slist_stat(*args,**kwargs):pass


    @classmethod
    @cache_api(ONE)
    def list_range(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_pop(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_push(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_rpop(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_rpush(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_insert(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_insert_before(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_insert_after(*args,**kwargs):pass


    @classmethod
    @cache_api(ALL)
    def list_set(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_del(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_del_val(*args,**kwargs):pass


    @classmethod
    @cache_api(ONE)
    def list_index(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_clear(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def list_destroy(*args,**kwargs):pass

    @classmethod
    @cache_api(ONE)
    def list_len(*args,**kwargs):pass

    @classmethod
    @cache_api(ONE)
    def list_stat(*args,**kwargs):pass

#Alias for KeyListCache
KLC=KeyListCache

class KeyValueCache(object):


    @classmethod
    @cache_api(ONE, KV_READ)
    def kv_get(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL, KV_WRITE)
    def kv_set(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def kv_del(*args,**kwargs):pass

    @classmethod
    @cache_api(ONE, KV_READ)
    def kv_get_multi(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL, KV_WRITE)
    def kv_set_multi(*args,**kwargs):pass

    @classmethod
    @cache_api(ALL)
    def kv_del_multi(*args,**kwargs):pass

#Alias for KeyValueCache
KVC=KeyValueCache
