#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def get_depends(tar):
    # Get the names of dependencies in embedded library
    prefix = "pak/src/library/"
    for member in tar.getmembers():
        name = member.name
        if not name.startswith(prefix) or not member.isdir():
            continue
        name = name[len(prefix):]
        if "/" not in name:
            yield name

def pre_build():
    r_pre_build(
        _G,
        expect_needscompilation = True,
        extra_r_depends_cb = get_depends,
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
