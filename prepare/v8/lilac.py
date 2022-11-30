#!/usr/bin/env python3
from lilaclib import *

def pre_build():
    update_pkgver_and_pkgrel(_G.newver.lstrip('v'))
    run_cmd(['updpkgsums'])
    # run 6 jobs in parallel on our server.
    run_cmd(['sh', '-c', "sed -i 's|ninja|ninja -j6|g' PKGBUILD"])

def post_build():
    # don't fix threads in PKGBUILD.
    run_cmd(['sh', '-c', "sed -i 's|ninja -j6|ninja|g' PKGBUILD"])
    git_add_files('PKGBUILD')
    git_commit()
