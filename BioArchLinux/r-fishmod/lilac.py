#!/usr/bin/env python3

from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(_G)

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
