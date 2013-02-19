# -*- coding: utf-8 -*-
# author : KDr2

from hcache_config import TypeInfo

class Model(object):

    def __init__(self, tname, attrs, dirty_keys):
        self.type_name = tname
        tid = TypeInfo.model_type_id(self.type_name)
        self.fullname = (attrs["id"]<<8) | tid
        self.attributes = attrs
        self.dirty_keys = dirty_keys


    def get(self, key, default=None):
        if key in ['fullname', 'left', 'right']:
            return getattr(self, key, default)
        return self.attributes.get(key, default)

    def to_dict(self):
        ret = {}
        ret['type_name'] = self.type_name
        ret['fullname'] = self.fullname
        ret['attributes'] = self.attributes
        ret['dirty_keys'] = self.dirty_keys
        return ret


class Entity(Model):

    def __init__(self, tname, attrs, dirty_keys):
        Model.__init__(self, tname, attrs, dirty_keys)

class Relation(Model):

    def __init__(self, tname, attrs, dirty_keys, left, right):
        Model.__init__(self, tname, attrs, dirty_keys)
        self.left = left
        self.right = right

    def to_dict(self):
        ret = Model.to_dict(self)
        ret['left'] = self.left
        ret['right'] = self.right
        return ret

    def rel_cache_key(self):
        tid = TypeInfo.model_type_id(self.type_name)
        return "R%d_%d_%d" % (tid, left, right)

    def rrel_cache_key(self):
        tid = TypeInfo.model_type_id(self.type_name)
        return "R%d_%d_%d" % (tid, right, left)
