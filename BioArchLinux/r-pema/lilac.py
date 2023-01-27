#!/usr/bin/env python3
from lilaclib import *
from os import environ

def pre_build():
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver='):
            line = f'_pkgver={_G.newver}'
        print(line)
    update_pkgver_and_pkgrel(_G.newver.replace(':', '.').replace('-', '.'))
    # each thread consumes about 2GiB memory
    environ.setdefault("MAKE", "make -j5")
