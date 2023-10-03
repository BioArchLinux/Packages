#!/usr/bin/env python3
from lilaclib import *

def pre_build():    
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver2='):
            line = f'_pkgver2={_G.newvers[1]}'
        print(line)

    update_pkgver_and_pkgrel(_G.newver)
