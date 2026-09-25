#!/usr/bin/env python3
from lilaclib import run_protected

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_update_pkgver_and_pkgrel

def pre_build():
    r_update_pkgver_and_pkgrel(_G.newver)
    run_protected(['updpkgsums'])
