#!/usr/bin/env python3
from lilaclib import *


def apply_patch(filename, patch,reverse=False):
    rev="-R" if revert else ""
    patch_proc = subprocess.Popen(
        ["patch", "-p1", rev, filename], stdin=subprocess.PIPE, text=True)
    patch_proc.communicate(patch)

def pre_build():
    update_pkgver_and_pkgrel(_G.newver.lstrip('v'))
    run_cmd(['updpkgsums'])
    apply_patch('PKGBUILD', PATCH)

def post_build():
    apply_patch('PKGBUILD', PATCH, reverse=True)
    git_add_files('PKGBUILD')
    git_commit()

PATCH = r"""
diff --git a/BioArchLinux/libsbml/PKGBUILD b/BioArchLinux/libsbml/PKGBUILD
index 2cf9189a0..e19634ec2 100644
--- a/BioArchLinux/libsbml/PKGBUILD
+++ b/BioArchLinux/libsbml/PKGBUILD
@@ -34,7 +34,7 @@ build() {
     -DENABLE_FBC=ON \
     -DENABLE_GROUPS=ON

-  make
+  make -j6
 }

 package() {
"""
