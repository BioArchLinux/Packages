#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "libmariadb-client-dev | libmariadb-client-lgpl-dev | libmysqlclient-dev (deb), mariadb-devel (rpm), mariadb | mysql-connector-c (brew), mysql56_dev (csw)",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
