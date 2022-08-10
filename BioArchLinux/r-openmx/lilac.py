#!/usr/bin/env python3
from lilaclib import *


def apply_patch(filename, patch, reverse=False):
    rev = "-R" if reverse else ""
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
diff --git b/BioArchLinux/r-openmx/PKGBUILD a/BioArchLinux/r-openmx/PKGBUILD
index 65c28dbe3..7c7930728 100644
--- b/BioArchLinux/r-openmx/PKGBUILD
+++ a/BioArchLinux/r-openmx/PKGBUILD
@@ -46,6 +46,8 @@ source=("https://cran.r-project.org/src/contrib/${_pkgname}_${_pkgver}.tar.gz")
 sha256sums=('65c50ce09f9c006b41b7311ec05eba3ae77926d84fb44e3905905208404826ed')

 build() {
+  # restrict the usage of cpu. Usage of RAM is OK.
+  export MAKE="make -j5"
   R CMD INSTALL ${_pkgname}_${_pkgver}.tar.gz -l "${srcdir}"
 }

"""
