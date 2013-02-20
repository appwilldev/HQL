# -*- coding: utf-8 -*-
# author : KDr2

import simplejson as json
from django.db import models

import hcache_model as hcmodel
import hcache_utils as utils
from hcache_config import TypeInfo
from hcache_hql import HQL
import hcache_lm
import hcache_capi

#extdata getter
def getter(key, to_dict = False):
    if isinstance(key, (str, unicode)):
        if key.isdigit(): key = long(key)
        elif key.startswith("R"):
            return hcache_capi.KVC.get(key)
    if isinstance(key, (int, long)):
        tid = key & 0xff
        tname = TypeInfo.type_name(tid)
        mclz =  TypeInfo.__TYPE_CLASSES__.get(tname, None)
        mid = key >> 8
        m = None
        if mclz._hcache_kv:
            m = hcache_capi.KVC.kv_get(key)
            if m and to_dict: return json.loads(m)
            if m and not to_dict: return hcmodel.Model.from_dict(jons.loads(m))
        if not m:
            try:
                m = mclz.objects.get(id=mid)
            except:
                pass
        if not m:
            return {} if to_dict else None
        return m.hcmodel().to_dict() if to_dict else m.hcmodel()
    return {} if to_dict else None


class HCManager(models.Manager):

    def __init__(self):
        models.Manager.__init__(self)

    def get_list_by_hql(self, hql):
        mclz = self.model #Model Class
        print mclz
        #TODO

#----- methods for Model

def _method_init(self, *args, **kwargs):
    models.Model.__init__(self, *args, **kwargs)
    self._hcmodel = self.hcmodel()

def _method_test_xmatch(self):
    m = self.hcmodel()
    extdata = utils.extdata_collector(m, getter)
    return HQL.xmatch(m.to_dict(), extdata)

def _method_save(self, force_insert=False, force_update=False, using=None):
    old_xmtach_result = None
    if self.id: # udpate, not create
        extdata = utils.extdata_collector(self._hcmodel, getter)
        old_xmtach_result = HQL.xmatch(self._hcmodel.to_dict(), getter)
    else:
        old_xmtach_result = {}

    models.Model.save(self,
                      force_insert = force_insert,
                      force_update = force_update,
                      using = using)

    self._hcmodel = self.hcmodel()


    self_fn = self.fullname()

    self_dict = self._hcmodel.to_dict()
    if(self._hcache_kv):
        hcache_capi.KVC.kv_set(self_fn, json.dumps(self_dict))

    tname = self.__class__.__name__.lower()
    tid = TypeInfo.model_type_id(tname)
    if tid>0x80:
        rkey = self._hcmodel.rel_cache_key()
        rels = hcache_capi.KVC.kv_get(rkey)
        rels = rels if rels else "[]"
        rels = json.loads(rels)
        if self_fn not in rels:
            rels.append(self_fn)
            hcache_capi.KVC.kv_set(rkey,json.dumps(rels))

    extdata = utils.extdata_collector(self._hcmodel, getter)
    new_xmtach_result = HQL.xmatch(self._hcmodel.to_dict(), getter)
    #TODO manage list and kv and rels
    print "old: ", old_xmtach_result
    print "new: ", new_xmtach_result
    fns = set(old_xmtach_result.keys()) | set(new_xmtach_result.keys())

    for fn in fns:
        oldls = old_xmtach_result.get(fn,[])
        newls = new_xmtach_result.get(fn,[])
        #add to new list:
        for x in newls:
            ae = x in oldls
            dirty_keys = self_dict['dirty_keys'] if x == self_fn else None
            hcache_lm.list_push(x, self_fn, self._hcmodel,
                                dirty_keys, ae, getter)
        #del from old
        for y in oldls:
            if y in newls: continue
            hcache_lm.list_del(y, self_fn)


def _method_delete(self, using=None):
    self_fn = self.fullname()
    hcm = self._hcmodel
    self_dict = self._hcmodel.to_dict()

    old_xmtach_result = None
    new_xmtach_result = None
    if self.id: # udpate, not create
        extdata = utils.extdata_collector(self._hcmodel, getter)
        old_xmtach_result = HQL.xmatch(self_dict, getter)
        self_dict["attributes"]["deleted"] = True
        new_xmtach_result = HQL.xmatch(self_dict, getter)
    else:
        old_xmtach_result = {}
        new_xmtach_result = {}

    delattr(self, "_hcmodel")
    models.Model.delete(self, using = using)

    if(self._hcache_kv):
        hcache_capi.KVC.kv_del(self_fn)

    tname = self.__class__.__name__.lower()
    tid = TypeInfo.model_type_id(tname)
    if tid>0x80:
        rkey = hcm.rel_cache_key()
        rels = hcache_capi.KVC.kv_get(rkey)
        rels = rels if rels else "[]"
        rels = json.loads(rels)
        if self_fn in rels:
            rels.remove(self_fn)
            hcache_capi.KVC.kv_set(rkey,json.dumps(rels))

    #TODO manage list and kv
    fns = set(old_xmtach_result.keys()) | set(new_xmtach_result.keys())

    for fn in fns:
        oldls = old_xmtach_result.get(fn,[])
        newls = new_xmtach_result.get(fn,[])
        #add to new list:
        for x in newls:
            ae = x in oldls
            hcache_lm.list_push(x, self_fn, self._hcmodel, None, ae, getter)
        #del from old
        for y in oldls:
            if y in newls: continue
            hcache_lm.list_del(y, self_fn)

def _method_fullname(self):
    if not self.id: return 0
    mclz = self.__class__
    tid = TypeInfo.model_type_id(mclz.__name__.lower())
    return (self.id << 8) | tid

def _method_hcmodel(self):
    type_name = self.__class__.__name__.lower()
    attrs = {}
    for f in self.__class__._meta.fields:
        attrs[f.name] = getattr(self, f.name)
        if isinstance(attrs[f.name], models.Model):
            attrs[f.name] = attrs[f.name].fullname()
    #endfor

    dirty_keys = None
    if (not hasattr(self, "_hcmodel")):
        dirty_keys = []
    else:
        s1 = set([(k,v) for k,v in self._hcmodel.attributes.iteritems()])
        s2 = set([(k,v) for k,v in attrs.iteritems()])
        dirty_keys = [k for k,v in s2-s1]
    #endif

    tid = TypeInfo.model_type_id(type_name)
    if(tid<0x80):
        return hcmodel.Entity(type_name, attrs, dirty_keys)
    else:
        left_tid, right_tid = TypeInfo.relation_element_type(type_name)

        left = self.left
        if not isinstance(left, (int, long)) : left=left.pk
        left = (left<<8) | left_tid

        right = self.right
        if not isinstance(right, (int, long)) : right=left.pk
        right = (right<<8) | right_tid

        return hcmodel.Relation(type_name, attrs, dirty_keys, left, right)

class HCache(object):

    def __init__(self, klcache = True, kvcache = False):
        self._klcache = klcache
        self._kvcache = kvcache

    def __call__(self, mclz):
        mclz._hcache_kl = self._klcache
        mclz._hcache_kv = self._kvcache
        TypeInfo.register_class(mclz.__name__.lower(), mclz)

        mclz.__init__ = _method_init
        mclz.save = _method_save
        mclz.deleted = _method_delete
        mclz.fullname = _method_fullname
        mclz.hcmodel = _method_hcmodel
        mclz.test_xmatch = _method_test_xmatch

        for k, v in mclz.__dict__.iteritems():
            if not isinstance(v, HQL): continue
            v.register()
        #endfor

        return mclz
