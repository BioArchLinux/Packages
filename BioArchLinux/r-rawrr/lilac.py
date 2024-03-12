#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "mono-runtime 4.x or higher (including System.Data library) on Linux/macOS, .Net Framework (>= 4.5.1) on Microsoft Windows.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
