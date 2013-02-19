# -*- coding: utf-8 -*-
# author : KDr2

def extdata_collector(m, getter, extmd_info = None):
    if not extmd_info: extmd_info = HQL.extmd_info()
    ret = {}
    type_name = m.type_name
    if not type_name: return ret

    keys = extmd_info.get(type_name, {}).get("keys", [])
    for k in keys:
        fn = m.get(k)
        if fn: ret[fn] = getter(fn)
    #endfor

    rinfo = extmd_info.get(type_name, {}).get("relation_info", 0)
    if rinfo == 0: return ret

    if rinfo >= 1:
        rck = m.rel_cache_key()
        rels = getter(rck)
        if isinstance(rels, (str, unicode)) and len(rels)>2:
            rels = json.loads(rels)
            ret[rck] = rels
        else:
            ret[rck] = []
        #endif
    #endif

    if rinfo >= 2:
        rrck = m.rrel_cache_key()
        rels = getter(rrck)
        if isinstance(rels, (str, unicode)) and len(rels)>2:
            rels = json.loads(rels)
            ret[rck] = rels
            if len(rels) > 0: ret[str(rels[0])] = getter(rels[0])
        else:
            ret[rck] = []
    #endif

    return ret
