#!/usr/bin/env python3
from lilaclib import *

import os
import sys


def pre_build():
    r_pre_build(_G)

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
