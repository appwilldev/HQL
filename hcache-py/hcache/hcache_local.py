#!/usr/bin/env python
#-*- coding:utf-8 -*-
# author : KDr2



class CacheLocal(object):

    def __init__(self,name):
        self.CACHE_DICT={}
        self.name=name


    def kv_set(self,k,v):
        self.CACHE_DICT[k]=v
    

    def kv_get(self,k,default=None):
        return self.CACHE_DICT.get(k,default)


    def kv_del(self,key):
        if self.CACHE_DICT.has_key(key):
            del self.CACHE_DICT[key]

            
    def kv_set_multi(self,kv_dict, prefix=''):
        new_kv_dict = {}
        for k,v in kv_dict.iteritems():
            self.CACHE_DICT[str(prefix)+str(k)] = v


    def kv_get_multi(self, keys, prefix=''):
        new_keys = [str(prefix)+str(k) for x in keys]
        return dict([k,self.kv_get(k)] for k in new_keys)


    def kv_del_multi(self, keys, prefix=''):
        new_keys = [str(prefix)+str(k) for x in keys]
        for k in new_keys:
            self.kv_del(k)


    def kv_sget(self, k,default=None):
        if self.CACHE_DICT.has_key(k):
            return self.CACHE_DICT[k]
        self.CACHE_DICT[k]=default
        return default

    #key-list api

    def list_range(self, key, count, start_index, start_value=None):
        lst=self.kv_get(key,[])
        return lst[start_index:start_index+count]


    def list_pop(self,key):
        lst=self.kv_sget(key,[])
        if not lst:return None
        ret=lst[0]
        lst.remove(ret)
        return ret

    def list_push(self, key, value):
        lst=self.kv_sget(key,[])
        lst.insert(0,value)
        return value

    def list_push_multi(self, key, values):
        lst=self.kv_sget(key,[])
        lst[0:1]=values
        return values

    def list_rpop(self,key):
        lst=self.kv_sget(key,[])
        if not lst:return None
        return lst.pop()

    def list_rpush(self, key, value):
        lst=self.kv_sget(key,[])
        lst.append(value)
        return value

    def list_rpush_multi(self, key, values):
        lst=self.kv_sget(key,[])
        lst.extend(values)
        return values

    def list_insert(self, key, index, value_to_insert):
        lst=self.kv_sget(key,[])
        lst.insert(index,value_to_insert)
        return value_to_insert


    def list_insert_before(self,key, value, value_to_insert):
        #TODO
        raise NotImplementedError


    def list_insert_after(self, key, value, value_to_insert):
        #TODO
        raise NotImplementedError

    def list_set(self, key, index, value):
        lst=self.kv_sget(key,[])
        lst[index]=value
        return value

    def list_del_val(self, key, value):
        lst=self.kv_sget(key,[])
        lst.remove(value)
        return value

    def list_index(self, key, index):
        lst=self.kv_sget(key,[])
        if len(lst)>index:
            return lst[index]
        return None

    def list_clear(self,key):
        self.kv_set(key,[])

    def list_destroy(self,key):
        if self.CACHE_DICT.has_key(key):
            del self.CACHE_DICT[key]
        return True

    def list_len(self,key):
        lst=self.kv_sget(key,[])
        return len(lst)    

    def list_stat(self,key):
        raise NotImplementedError


