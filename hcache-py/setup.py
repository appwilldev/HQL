#!/usr/bin/env python
#-*- coding:utf-8 -*-
# author : KDr2


import os
import re
import sys
import glob

from distutils.core import setup, Extension


src_path=os.path.abspath(os.path.dirname(__file__))
mod_files=glob.glob(os.path.join(src_path,"hcache/*.py"))
re_name=re.compile(".*/([^/]+?)\.py")
hcmods=[]

for f in mod_files:
    m=re_name.match(f)
    if not m:continue
    #if m.group(1) == '__init__':continue
    hcmods.append("hcache.%s"%m.group(1))


setup(name = 'hcache',
      version = '0.1',
      description = 'HCache By KDr2.',
      py_modules=hcmods)
