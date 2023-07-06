#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "Package vignettes based on R Markdown v2 or reStructuredText require Pandoc (http://pandoc.org). The function rst2pdf() requires rst2pdf (https://github.com/rst2pdf/rst2pdf).",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
