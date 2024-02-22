#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "A C++17 compiler is required, and for macOS compilation version 11.0 or later is required. Optionally cmake (only when TileDB source build selected), curl (only when TileDB source build selected)), and git (only when TileDB source build selected); on x86_64 and M1 platforms pre-built TileDB Embedded libraries are available at GitHub and are used if no TileDB installation is detected, and no other option to build or download was specified by the user.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
