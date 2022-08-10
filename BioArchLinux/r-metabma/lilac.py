#!/usr/bin/env python3
from lilaclib import *


def apply_patch(filename, patch, reverse=False):
    rev = "-R" if revert else ""
    patch_proc = subprocess.Popen(
        ["patch", "-p1", rev, filename], stdin=subprocess.PIPE, text=True)
    patch_proc.communicate(patch)

def pre_build():
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver='):
            line = f'_pkgver={_G.newver}'
        print(line)
    update_pkgver_and_pkgrel(_G.newver.replace(':', '.').replace('-', '.'))
    apply_patch('PKGBUILD', PATCH)

def post_build():
    apply_patch('PKGBUILD', PATCH, reverse=True)
    git_pkgbuild_commit()
    update_aur_repo()


PATCH = r"""
diff --git b/BioArchLinux/r-metabma/PKGBUILD a/BioArchLinux/r-metabma/PKGBUILD
index e1ec269a6..65298aefc 100644
--- b/BioArchLinux/r-metabma/PKGBUILD
+++ a/BioArchLinux/r-metabma/PKGBUILD
@@ -36,6 +36,8 @@ source=("https://cran.r-project.org/src/contrib/${_pkgname}_${_pkgver}.tar.gz")
 sha256sums=('330bccb4b2297bc3a8b7291197c5e978b90b002907f762ede40f2d3e383367da')

 build() {
+  # restrict the usage of memory and cpu, 1 threads usually consumes 2 GiB memory.
+  export MAKE="make -j5"
   R CMD INSTALL ${_pkgname}_${_pkgver}.tar.gz -l "${srcdir}"
 }
"""
