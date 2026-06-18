#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_update_pkgver_and_pkgrel

def pre_build():
    r_update_pkgver_and_pkgrel(_G)

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
