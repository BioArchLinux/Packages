#!/usr/bin/env python3
from lilaclib import *


def pre_build():
    for line in edit_file('PKGBUILD'):
        if line.startswith('_commit='):
            line = f'_commit={_G.newver}'
        print(line)

    update_pkgrel()
    run_cmd(['updpkgsums'])


def post_build():
    git_pkgbuild_commit()
    update_aur_repo()
