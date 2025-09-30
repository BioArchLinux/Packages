#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "ADMIXTOOLS suite of command-line utilities for population genetics. See <https://reich.hms.harvard.edu/software> for the most recent installation instructions and further information."
    )


def post_build():
    git_pkgbuild_commit()
