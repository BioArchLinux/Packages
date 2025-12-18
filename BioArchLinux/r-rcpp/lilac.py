#!/usr/bin/env python3
#from lilaclib import *

#import os
#import sys
#sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
#from lilac_r_utils import r_pre_build

from lilaclib import *

def pre_build():
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver='):
            line = f'_pkgver={_G.newver}'
        print(line)
    update_pkgver_and_pkgrel(_G.newver.replace(':', '.').replace('-', '.'))
    
#def#def pre_buil
#    r_pre_build(_G)

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()

