#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "C++17; for AWS S3 support on Linux, libcurl and openssl (optional); cmake >= 3.25 (build-time only, and only for full source build)",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
