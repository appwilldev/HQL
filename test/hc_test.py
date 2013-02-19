#!/usr/bin/env python

import sys
sys.path.append("..")
sys.path.append("../hcache-py")
import hcache

f=open("../test/type.json")
hcache.HQL.setup_config(f.read())
f.close()

import test_data


h1=hcache.HQL('select user where id > 100');



s='select user where tag contains ? by "," and select user where tag contains ? by ","  and select user where tag contains ? by "," limit 12'
h2=hcache.HQL(s)

print h1.register()
print h2.register()
print hcache.HQL.all_hql()
print hcache.HQL.extmd_info()
