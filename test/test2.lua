#!/usr/bin/env lua


package.cpath = "../?.so;" .. package.cpath;
hql = require('hql')

---
--- Utils
---

function table_real_length(t)
    local count = 0
    for k,v in pairs(t) do
        count = count + 1
    end
    return count
end

function _strify(o, tab, act, logged)
    local v=tostring(o)
    if logged[o] then return v end
    if string.sub(v,0,6) == "table:" then
        logged[o]=true
        act = "\n" .. string.rep("|    ",tab) .. "{ [".. tostring(o) .. ", "
        act = act .. table_real_length(o) .." item(s)]"
        for k,v in pairs(o) do
            act =  act .."\n" .. string.rep("|    ",tab)
            act =  act .. "|   *".. k .. "\t=>\t" .. _strify(v,tab+1,act, logged)
        end
        act= act .. "\n"..string.rep("|    ",tab) .."}"
        return act
    else
        return v
    end
end

function strify(o) return _strify(o,1,"",{}) end

function table_print(t)
    local s1="\n* Table String:"
    local s2="\n* End Table"
    return (s1 .. strify(t) .. s2)
end

str = table_print


---
--- setup type config
---

require("io")
local config = io.open("type.json", "r")
local types = config.read(config, "*all")
hql.setup_config(types)

require("test_data")

---
--- Register Trollers
---

print(hql.hql2hql('select user where id > 100'));
print(hql.hql2cachekey('select user where id > 100'));

s='select user where tag contains ? by "," and select user where tag contains ? by ","  and select user where tag contains ? by "," limit 12'
hql.register_troller(s)
print(hql.hql2cachekey(s))
print(hql.hql2cachekey(s,true))
print(hql.hql2cachekey(s,false))
print('>>>', str(hql.xmatch(test_data.u1, test_data.database)))
