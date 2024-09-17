#!/usr/bin/env python3
from lilaclib import *

# we use 5 threads in BioArchLinux, but it should not be pushed to aur.

def pre_build():
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver='):
            line = f'_pkgver={_G.newver}'
        elif line.strip().startswith("ninja") and not "DESTDIR" in line:
            line += ' -j5'
        print(line)
    update_pkgver_and_pkgrel(_G.newver.replace(':', '.').replace('-', '.'))


def post_build():
    run_cmd(["sh", "-c", "sed -i '/ninja/s/ -j5//g' PKGBUILD"])
    git_pkgbuild_commit()
    update_aur_repo()
