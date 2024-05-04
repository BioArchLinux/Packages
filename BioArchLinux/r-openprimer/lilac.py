#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "MAFFT (>= 7.305), OligoArrayAux (>= 3.8), ViennaRNA (>= 2.4.1), MELTING (>= 5.1.1), Pandoc (>= 1.12.3)",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
