#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(_G,
            expect_systemrequirements = "Optional: libsecret on Linux (libsecret-1-dev on Debian/Ubuntu, libsecret-devel on Fedora/CentOS)",
            )

def post_build():
    git_pkgbuild_commit()
