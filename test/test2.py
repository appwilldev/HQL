#!/usr/bin/env python

import sys
sys.path.append("..")
import hql
f=open("type.json")
hql.setup_config(f.read())
f.close()

import test_data


# Register Trollers

class X:
    pass

print(hql.hql2hql('select user where id > 100 order by abc'));
print(hql.hql2cachekey('select user where id > 100'));

print(hql.register_hql('select user where id > 100 order by abc'));
print(hql.register_hql('select user where id > 1001'));


s='select user where tag contains ? by "," and select user where tag contains ? by ","  and select user where tag contains ? by "," order by abc limit 12'
hql.register_troller(s)

def t():
    for i in range(20000):
        print "\n\n\n"
        print(hql.all_hql())
        print(hql.hql_info(s))
        print(hql.extmd_info())
        print(hql.hql2cachekey(s))
        print(hql.hql2cachekey(s,True))
        print(hql.hql2cachekey(s,False))
        #print('>>>', hql.xmatch(test_data.u1, test_data.database))

import thread
ts = []
for i in range(20):
    ts.append(thread.start_new_thread(t,()))

import time
time.sleep(300)
