#-*- coding:utf-8 -*-
# author : KDr2

import hcache_model
import hcache_capi
import hcache_hql
import hcache_utils as utils

HQL = hcache_hql.HQL

def dummy_getter(k, to_dict=True):
    if to_dict: return {}
    return None

def list_push(ld, fn, m, dirty_keys=None, already_exists=False, getter = dummy_getter):
    if not dirty_keys: dirty_keys = []
    info = HQL.keyinfo(ld)
    ld = utils.trim_info_ld(ld)
    if info.get('order_key', None):
        ordered_push(ld, info, fn, m,
                     dirty_keys = dirty_keys,
                     already_exists = already_exists,
                     getter = getter)
    else:
        natrue_order_push(ld, info, fn, already_exists=already_exists)


def natrue_order_push(ld, info, fn, already_exists=False):
    if already_exists: return

    #1. new value
    #logger.d("list_mgr: add    fullname:%d to list: %s" % (value,ld.cache_key()))
    hcache_capi.KLC.list_push(ld, fn, limit=info.get('limit', None))



def ordered_push(ld, info, fn, value, dirty_keys=None, already_exists=False,
                 getter = dummy_getter):
    if not dirty_keys: dirty_keys = []
    order_key = info.get("order_key")
    order_asc = info.get("roder_asc")

    if (order_key not in dirty_keys) and already_exists: return

    ob = (hcache_capi.ORDER_ASC if order_asc else hcache_capi.ORDER_DESC)

    order_model = None
    if order_key.startswith("guest."):
        order_model = getter(fn, to_dict=False)
        order_key = order_key[6:]
    elif order_key.startswith("host."):
        order_model = value
        order_key = order_key[5:]
    else:
        order_model = getter(fn, to_dict=False)
    #endif

    # del old value is exists
    #if already_exists:
        #hcache_capi.KLC.list_del_val(ld.cache_key(), fn)
    # find pos and insert
    #_ordered_push(ld.cache_key(), fn, order_key, order_asc)

    hcache_capi.KLC.slist_add(ld, fn, order_model.get(order_key,0),
                              limit = info.get("limit"),
                              order_by = ob)

def list_del(ld, fn):
    info = HQL.keyinfo(ld)
    ld = utils.trim_info_ld(ld)
    if info.get("order_key"):
        hcache_capi.KLC.slist_del_val(ld, fn)
    else:
        hcache_capi.KLC.list_del_val(ld, fn)
