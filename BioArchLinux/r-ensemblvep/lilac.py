#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "Ensembl VEP (API version 105) and the Perl modules DBI and DBD::mysql must be installed. See the package README and Ensembl installation instructions: http://www.ensembl.org/info/docs/tools/vep/script/vep_download.html#installer",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
