#!/usr/bin/env python

import sys
sys.path.append("..")
import hql
f=open("type.json")
hql.setup_config(f.read())
f.close()

import test_data


# Register Trollers


print(hql.hql2hql('select user where id > 100'));
print(hql.hql2cachekey('select user where id > 100'));


s='select user where tag contains ? by "," and select user where tag contains ? by ","  and select user where tag contains ? by "," limit 12'
hql.register_troller(s)
for i in range(99999):
    print(hql.all_hql())
    print(hql.hql2cachekey(s))
    print(hql.hql2cachekey(s,True))
    print(hql.hql2cachekey(s,False))
    print('>>>', hql.xmatch(test_data.u1, test_data.database))
