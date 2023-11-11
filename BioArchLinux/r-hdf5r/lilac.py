#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_license = "Apache License 2.0 | file LICENSE",
        expect_systemrequirements = "HDF5 (>= 1.8.13)",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
