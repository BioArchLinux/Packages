#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "Linux, Mac OSX, and Windows, or 'ZeroMQ' library >= 4.0.4. Solaris 10 needs 'ZeroMQ' library 4.0.7 and 'OpenCSW'.",
    )

def post_build():
    git_pkgbuild_commit()
