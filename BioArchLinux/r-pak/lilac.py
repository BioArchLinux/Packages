#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

import shutil, tarfile
from tempfile import NamedTemporaryFile

def pre_build():
    # Get the names of dependencies in embedded library
    deps = []
    newver = _G.newver.rsplit("#", 1)[0]
    with tarfile.open(f"pak_{newver}.tar.gz", "r:gz") as tar:
        prefix = "pak/src/library/"
        for member in tar.getmembers():
            name = member.name
            if not name.startswith(prefix) or not member.isdir():
                continue
            name = name[len(prefix):]
            if "/" not in name:
                deps.append(name)

    r_pre_build(
        _G,
        expect_needscompilation = True,
        extra_r_depends = deps,
    )

def post_build():
    git_pkgbuild_commit()
