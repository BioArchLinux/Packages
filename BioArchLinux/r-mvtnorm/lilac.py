#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import (
    r_pre_build, 
    all_checks, 
    check_default_pkgs,
    check_makedepends,
    check_optdepends,
    check_fortran,
    check_systemrequirements,
    check_license,
    check_pkgdesc,
    check_arch,
    check_md5sum,
)
import lilac_r_utils

# remove check_depends
lilac_r_utils.all_checks = [
    check_default_pkgs,
    # check_depends
    check_makedepends,
    check_optdepends,
    check_fortran,
    check_systemrequirements,
    check_license,
    check_pkgdesc,
    check_arch,
    check_md5sum,
]

def pre_build():
    r_pre_build(_G)

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
