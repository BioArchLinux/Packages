#!/usr/bin/env python3
from lilaclib import *

def pre_build():
  dt = _G.newver.split(None)
  dt = datetime.strptime(dt, '%d-%b-%Y %H:%M').strftime('%Y%m%d.%H')
    
  for line in edit_file('PKGBUILD'):
      line.startswith('pkgver='):
      line = f'pkgver={dt}'
    print(line)
    
    update_pkgver_and_pkgrel(f'{dt}')
    
    run_cmd(['updpkgsums'])

def post_build():
    git_add_files('PKGBUILD')
    git_commit()
