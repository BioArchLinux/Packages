#!/usr/bin/env python3
import os
import sys
sys.path.append(os.path.normpath(f"{__file__}/../../../lilac-extensions"))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(_G)
