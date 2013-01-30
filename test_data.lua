module("test_data", package.seeall)

---
--- Database And ModelGetter
---

u1= {type_name= "user",    fullname= 257,                        attributes= {cttr=50, age=20, tag="a,b,c,中1,中国,中日友好", ext= "d,e,f", b=true, name="h\"i", utc=1354607332.160434}}
u2= {type_name= "user",    fullname= 513,                        attributes= {cttr=50, age=20, tag="a,b,c", ext= "d,e,f", b=true}}
p1= {type_name= "post",    fullname= 259,                        attributes= {cttr=50, author_id=257, num_comments= 20, tag= "a,b,c", ext= "d,e,f"}}
p2= {type_name= "post",    fullname= 515,                        attributes= {cttr=50, author_id=513, num_comments= 20, tag= "a,b,c", ext= "d,e,f"}}
b1= {type_name= "board",   fullname= 258,                        attributes= {cttr=50, author_id=257, num_links=20, tag="a,b,c", ext= "d,e,f"}}
b2= {type_name= "board",   fullname= 514,                        attributes= {cttr=50, author_id=513, num_links=20, tag="a,b,c", ext= "d,e,f"}}

c1= {type_name= "comment", fullname= 385, left= 257, right= 259, attributes= {author_id=257, num_comments= 20, tag= "a,b,c"}}
c2= {type_name= "comment", fullname= 641, left= 513, right= 515, attributes= {author_id=513, num_comments= 20, tag= "a,b,c"}}
c3= {type_name= "comment", fullname= 897, left= 513, right= 515, attributes= {author_id=513, num_comments= 20, tag= "a,b,c"}}

f1= {type_name= "follow",  fullname= 386, left= 257, right= 513, attributes= {tag= "a,b,c"}}
f2= {type_name= "follow",  fullname= 642, left= 513, right= 257, attributes= {tag= "a,b,c", deleted=false}}


database={ -- key-value storage


    ["257"]=u1, ["513"]=u2,
    ["259"]=p1, ["515"]=p2,
    ["385"]=c1, ["641"]=c2, ["897"]=c3,
    ["386"]=f1, ["642"]=f2,
    ["R129_257_259"]={385 },
    ["R129_513_515"]={641, 897},
    ['R130_257_513']={386, },
    ['R130_513_257']={642, },
}


require("hql")
extmd_info = hql.extmd_info()

function extmd_collect(m, getter)
    local ret = {} -- fn={}, "Rn_n_n"={fns}
    local type_name = m.type_name
    if not type_name then return ret end
    if not extmd_info[type_name] then return ret end
    local keys = extmd_info[type_name].keys or {}
    for _, k in ipairs(keys) do
        local fn = nil
        if k == "left" or k == "right" then
            fn = m[k]
        else
            --fn = m:get(k)
            fn = m.attributes[k]
        end
        ret[fn] = getter(fn) or {}
    end
    -- if m is not relation return ret
    local rinfo = extmd_info[type_name].relation_info or 0
    if rinfo >= 1 then

    end
    if rinfo >= 2 then

    end
    return ret
end
