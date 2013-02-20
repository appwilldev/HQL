# -*- coding: utf-8 -*-
# author : KDr2

from django.db import models
import hcache_model as hcmodel
import hcache_utils as utils
from hcache_config import TypeInfo
from hcache_hql import HQL

#extdata getter
def getter(key):
    if isinstance(key, (str, unicode)):
        if key.isdigit(): key = long(key)
        elif key.startswith("R"):
            #TODO
            return None
    if isinstance(key, (int, long)):
        tid = key & 0xff
        tname = TypeInfo.type_name(tid)
        mclz =  TypeInfo.__TYPE_CLASSES__.get(tname, None)
        mid = key >> 8
        m = mclz.object.find(id=mid)
        return m.hcmodel().to_dict() if m else None
    return None


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
    extdata = utils.extdata_collector(self._hcmodel, getter)
    new_xmtach_result = HQL.xmatch(self._hcmodel.to_dict(), getter)
    #TODO manage list and kv
    print "old: ", old_xmtach_result
    print "new: ", new_xmtach_result

def _method_delete(self, using=None):
    old_xmtach_result = None
    if self.id: # udpate, not create
        extdata = utils.extdata_collector(self._hcmodel, getter)
        old_xmtach_result = HQL.xmatch(self._hcmodel.to_dict(), getter)
    else:
        old_xmtach_result = {}
    delattr(self, "_hcmodel")
    models.Model.delete(self, using = using)
    #TODO manage list and kv

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
