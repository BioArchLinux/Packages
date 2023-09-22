#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "By default, git2r uses a system installation of the libgit2 headers and library. However, if a system installation is not available, builds and uses a bundled version of the libgit2 source. zlib headers and library. OpenSSL headers and library (non-macOS). LibSSH2 (optional on non-Windows) to enable the SSH transport.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
