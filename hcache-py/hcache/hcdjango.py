# -*- coding: utf-8 -*-
# author : KDr2

import hcache_model as hcmodel
from django.db import models
from hcache_config import TypeInfo

class HCManager(models.Manager):
    pass

class HCModel(models.Model):

    objects = HCManager()

    class Meta:
        #proxy = True
        abstract = True

    def __init__(self, *args, **kwargs):
        models.Model.__init__(self, *args, **kwargs)
        self._hcmodel = self.hcmodel()

    def save(self, force_insert=False, force_update=False, using=None):
        #TODO manage list and kv
        print self.hcmodel().to_dict()
        models.Model.save(self,
                          force_insert = force_insert,
                          force_update = force_update,
                          using = using)
        self._hcmodel = self.hcmodel()

    def delete(self, using=None):
        #TODO manage list and kv
        delattr(self, "_hcmodel")
        models.Model.delete(self, using = using)

    def hcmodel(self):
        type_name = self.__class__.__name__.lower()
        attrs = {}
        for f in self.__class__._meta.fields:
            attrs[f.name] = getattr(self, f.name)
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
