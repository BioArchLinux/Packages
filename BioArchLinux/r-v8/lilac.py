#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "On Linux you can build against libv8-dev (Debian) or v8-devel (Fedora). We also provide static libv8 binaries for most platforms, see the README for details.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
