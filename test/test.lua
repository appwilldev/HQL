_tostring = require("logging").tostring
require("hql")

function getter(fn)
    print("GETTER : " .. tostring(fn))
    return '{"id":123, "author": 5}'
end

hql.set_getter("getter")

function show_hql(s)
    print("=>USR: " .. s)
    local h=hql.format_hql(s)
    if h then
        print("=>HQL: " .. h)
    else
        print "ERROR"
    end


    local h=hql.hql2cachekey(s)
    if h then
        print("=>HQL: " .. h)
    else
        print "ERROR"
    end

    
    print("\n")
end


print(hql.hql2cachekey('select follow between *@ and @'))

----[[
s="select user where age > 10"
hql.register_troller(s)
show_hql(s)



s="select user where age = ? " -- ? <=> EACH
show_hql(s)



s="select user where age in [18,19,20] " -- ? <=> EACH
show_hql(s)

s="select user where gender in [\"bisexuality\", \"female\"] " -- ? <=> EACH
show_hql(s)

s="select user where age contains ? by \",\" " -- 
show_hql(s)

s="select post.author_id as user"
show_hql(s)

s="select post.author_id as user where num_comments>0"
show_hql(s)

s='select user where age>18 and select user where gender="female"'
show_hql(s)

s="not not not not select post.author_id as user"
show_hql(s)

s="NOT not not not not select post.author_id as user"
show_hql(s)

s="select comment between *@ and ?"
show_hql(s)

s="select comment between *@ and ?  order by name limit 10"
show_hql(s)

s="select comment between *@ and #68121  order by name limit 10"
show_hql(s)


s="select follow between *@ and ?  and select follow between ? and *@"
show_hql(s)

s="select follow between select *user and ? "
show_hql(s)


s="select follow between select *user and ? and  select follow between ? and select     * user"
show_hql(s)

s="(not (select user where age <18 )) and (not select user where age >40)"
show_hql(s)

s="(select user where age <18 ) and (select user where age >40) and (select user where id>20)"
show_hql(s)


s=" not (not (select user where age <18 )) or (not select user where age >40)"
show_hql(s)

s='( select user where age>=18) and (  select user where gender="male") and ( select user where num_posts>0)'
show_hql(s)


s='(not select user where age>=18) and ( not select user where gender="male") and (not select user where num_posts>0)'
show_hql(s)

s='(not select user where age>=18) or ( not select user where gender="male") or (not select user where num_posts>0)'
show_hql(s)

s='(not select user where age>=18) or ( not select user where gender="male") or not (not select user where num_posts>0)'
show_hql(s)

s='( not select user where gender="male") or (not (not select user where num_posts>0)) or (not select user where age>=18) limit 100 '
show_hql(s)

hql.format_hql('@ between @ and @')

s='select "error"' --error hql test
show_hql(s)

print("=== xmatch test ===\n")
print("should be empty tables:")
print(_tostring(hql.xmatch('{}', 'selet user')))
print(_tostring(hql.xmatch('}', 'select user')))
print(_tostring(hql.xmatch('{}', 'select user')))
print(_tostring(hql.xmatch('{}', 'select user where age > 4')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12}', 'select comment')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12}', 'select user where age > 4')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "post_id": 31}', 'select user.post_id as post where author = 6')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 31}', 'select user.post_id as post')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 2}', 'select user where age > 4')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": "ah"}', 'select user where age > 4')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 31}', 'select user where age > 4 and select user where age < 10')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 7}', 'not select user where age = 7')))

print("\nshould be ok:")
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12}', 'select user')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12}', 'select user order by age')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12}', 'select user order by age limit 3')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "post_id": 31}', 'select user.post_id as post')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "post_id": 31}', 'select user.post_id as post where author = 5')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 31}', 'select user where age > 4')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 31}', 'select user where age > 4 or select user where age < 10')))
print(_tostring(hql.xmatch('{"type": "user", "fullname": 12, "age": 7}', 'select user where age > 4 and select user where age < 10')))
