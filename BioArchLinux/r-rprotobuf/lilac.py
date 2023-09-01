#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "ProtoBuf libraries and compiler version 3.3.0 or later; On Debian/Ubuntu these can be installed as libprotoc-dev, libprotobuf-dev and protobuf-compiler, while on Fedora/CentOS protobuf-devel and protobuf-compiler are needed. A C++17 compiler is required as well.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
