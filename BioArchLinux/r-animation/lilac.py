#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(
        _G,
        expect_systemrequirements = "ImageMagick (http://imagemagick.org) or GraphicsMagick (http://www.graphicsmagick.org) or LyX (http://www.lyx.org) for saveGIF(); (PDF)LaTeX for saveLatex(); SWF Tools (http://swftools.org) for saveSWF(); FFmpeg (http://ffmpeg.org) or avconv (https://libav.org/avconv.html) for saveVideo()",
    )

def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
