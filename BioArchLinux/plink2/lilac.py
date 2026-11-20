#!/usr/bin/env python3
from lilaclib import *

def pre_build():
    newver = _G.newver
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver='):
            line = f'_pkgver={newver}'
        print(line)
    update_pkgver_and_pkgrel(newver.replace('-', ''))

def post_build():
    git_pkgbuild_commit()
