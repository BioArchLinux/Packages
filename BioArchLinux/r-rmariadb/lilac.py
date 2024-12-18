#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "libmariadb-client-lgpl-dev or libmariadb-dev or libmysqlclient-dev, with libssl-dev (deb), mariadb-connector-c-devel or mariadb-devel (rpm), mariadb-connector-c or mysql-connector-c (brew)",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
