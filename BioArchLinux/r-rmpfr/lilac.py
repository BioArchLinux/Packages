#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "gmp (>= 4.2.3), mpfr (>= 3.1.0), pdfcrop (part of TexLive) is required to rebuild the vignettes.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
