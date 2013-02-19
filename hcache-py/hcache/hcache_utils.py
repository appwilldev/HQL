# -*- coding: utf-8 -*-
# author : KDr2

def extdata_collector(m, getter, extmd_info = None):
    if not extmd_info: extmd_info = HQL.extmd_info()
    ret = {}
    type_name = m.get("type_name", None)
    if not type_name: return ret

    keys = extmd_info.get(type_name, {}).get("keys", [])
    for k in keys:
        fn = None
        if k in ["left", "right"]:
            fn = m.get(k, None)
        else:
            fn = m.get("atrributes",{}).get(k, None)
        #endif
        if fn: ret[fn] = getter(fn)
    #endfor

    rinfo = extmd_info.get(type_name, {}).get("relation_info", 0)
    if rinfo == 0: return ret

    if rinfo >= 1:
        #TODO
        pass
    #endif

    if rinfo >= 2:
        #TODO
        pass
    #endif

    return ret
