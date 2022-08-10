#!/usr/bin/env python3
from lilaclib import *

# for our server, we have to use make -j6, but it should not appears in the PKGBUILD
def pre_build():
    update_pkgver_and_pkgrel(_G.newver.lstrip('v'))
    run_cmd(['updpkgsums'])
    for line in edit_file('PKGBUILD'):
        if line.strip() == 'make':
            line = line + ' -j6'
        print(line)

def post_build():
    for line in edit_file('PKGBUILD'):
        if line.endswith('make -j6'):
            line = line[:-4]
        print(line)
    git_add_files('PKGBUILD')
    git_commit()
