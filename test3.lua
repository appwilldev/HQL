#!/usr/bin/env lua

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
s='select user where utc time_in 123'
hql.register_troller(s)
s='select user where name ="h\\"i"'
hql.register_troller(s)
s="select user where age > 10 limit 10"
hql.register_troller(s)
s="select user order by host.id desc limit 102"
hql.register_troller(s)
s="select user where age = ?"
hql.register_troller(s)
s='select user where tag contains ? by ","'
hql.register_troller(s)
hql.unregister_troller(s)
print("!!!",hql.use_namespace(1))

s='select post.author_id as user'
hql.register_troller(s)
s='select post.author_id as user where num_comments = ?'
hql.register_troller(s,0)

s='select comment between *@ and ?'
hql.register_troller(s)

s='select comment between @ and *@'
hql.register_troller(s)

s='select comment where num_comments > 10 between ? and *@'
hql.register_troller(s)

s="not select user where age > 30 limit 10"
hql.register_troller(s)


s="select user where age > 10 or select user where age <3"
hql.register_troller(s)


s="select user where age > 3 and select user where age <30"
hql.register_troller(s)
s='(select user where  tag contains ? by ",") and (select user where age = ?) and select user where  ext contains ? by ","'
--s='select user where  tag = ? and select user where age = ?'
hql.register_troller(s)

s='select comment between  ( select user where  tag contains ? by "," and select user where age = ? and select user where  ext contains ? by ",") and (select *post where num_comments >0)'
hql.register_troller(s)


s='select post.author_id as user and select user where age > 10'
hql.register_troller(s)

s='select post.author_id as user  and select post where tag contains ? by "," and select post where num_comments>10'
hql.register_troller(s)

s='select follow between ? and select *user where age>10'
hql.register_troller(s)


s='(select follow between ? and *@) and (select follow between select *user where age>10 and ?)'
hql.register_troller(s)

s='select thing'
hql.register_troller(s)
s='select thing where fullname > 0 and select thing where cttr>30 order by id limit 1000'
hql.register_troller(s)

s='select thing.author_id as user'
hql.register_troller(s)
s='select thing.author_id as user and select thing where cttr>30 order by id limit 1000'
hql.register_troller(s)

s='select user where b=true'
hql.register_troller(s)
print(hql.hql2hql(s))


print("!!!",hql.use_namespace(0))

-- List Trollers:
print('All Trollers:', str(hql.trollers()))
print('')

-- test xmatch
print("Test xMatch =====================")
print('')

print('>>>', str(hql.xmatch(test_data.u1, test_data.database)))
print('>>>', str(hql.xmatch(test_data.p1, test_data.database)))
print('>>>', str(hql.xmatch(test_data.b2, test_data.database)))
print('>>>', str(hql.xmatch(test_data.c2, test_data.database)))

print('>>>', str(hql.xmatch(test_data.f1, test_data.database)))
print('>>>', str(hql.xmatch(test_data.f2, test_data.database)))


print("HQL Info  =====================")
print('')

print('>>>', str(hql.hql_info("LIST[1]O[host.id,D,a[user]]L102")))
print('>>>', str(hql.hql_info("select user order by guest.name asc limit 10")))
--print('>>>', str(hql.hql_info("Lxxasasasasdasd,,,,")))
print('>>>', str(hql.hql_info("LIST[1]c[user.utc,TIME_IN,123000]")))

print('>>>', str(hql.hql_info("select user and select user where xutc time_in 1234")))
print(hql.hql2cachekey('select user where tag contains "a" by "," order by host.abc'))

print(str(hql.extmd_info()))
