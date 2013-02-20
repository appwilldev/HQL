# -*- coding: utf-8 -*-
# author : KDr2

from hcache_hql import HQL

def to_base(q, alphabet):
    if q < 0: raise ValueError, "must supply a positive integer"
    l = len(alphabet)
    converted = []
    while q != 0:
        q, r = divmod(q, l)
        converted.insert(0, alphabet[r])
    return "".join(converted) or '0'

def to36(q):
    if not q:return '0'
    if isinstance(q, (str, unicode)):q=int(q)
    return to_base(q, '0123456789abcdefghijklmnopqrstuvwxyz')

def force_int(i, default=0):
    try:
        return int(i)
    except:
        return default

def trim_info_ld(ld):
    index = ld.rindex("]")
    return ld[:index+1]


def extdata_collector(m, getter, extmd_info = None):
    if not extmd_info: extmd_info = HQL.extmd_info()
    ret = {}
    type_name = m.type_name
    if not type_name: return ret

    keys = extmd_info.get(type_name, {}).get("keys", [])
    for k in keys:
        fn = m.get(k)
        if fn: ret[fn] = getter(fn, to_dict=True)
    #endfor

    rinfo = extmd_info.get(type_name, {}).get("relation_info", 0)
    if rinfo == 0: return ret

    if rinfo >= 1:
        rck = m.rel_cache_key()
        rels = getter(rck, to_dict=True)
        if isinstance(rels, (str, unicode)) and len(rels)>2:
            rels = json.loads(rels)
            ret[rck] = rels
        else:
            ret[rck] = []
        #endif
    #endif

    if rinfo >= 2:
        rrck = m.rrel_cache_key()
        rels = getter(rrck, to_dict=True)
        if isinstance(rels, (str, unicode)) and len(rels)>2:
            rels = json.loads(rels)
            ret[rck] = rels
            if len(rels) > 0: ret[str(rels[0])] = getter(rels[0], to_dict=True)
        else:
            ret[rck] = []
    #endif

    return ret
