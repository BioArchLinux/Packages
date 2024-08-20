#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "FFmpeg (>= 3.2); with at least libx264 and lame (mp3) drivers. MacOS Homebrew: ffmpeg. Debian/Ubuntu: libavfilter-dev. Fedora/RHEL: either ffmpeg-devel from https://rpmfusion.org (preferred), or libavfilter-free-devel which is a more limited version available on Fedora/EPEL9.",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
