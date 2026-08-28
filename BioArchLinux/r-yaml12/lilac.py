#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "Cargo (Rust's package manager), rustc >= 1.71.0, xz. On Windows ARM64, source installs also require Microsoft C++ Build Tools with ARM64 components.",
    )

def post_build():
    git_pkgbuild_commit()
