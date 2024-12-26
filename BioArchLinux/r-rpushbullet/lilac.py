#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "A user API key (which one can request from the website at <http://www.pushbullet.com>), and one or more devices to push messages to which may be any one of an (Android or iOS) phone, a (Chrome or Firefox, or Opera or Safari) browser or the (Windows or Mac) desktop application provided the corresponding Pushbullet 'app' has been installed on any one of these.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
