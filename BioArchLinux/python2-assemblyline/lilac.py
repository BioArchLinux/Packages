#!/usr/bin/env python3

from lilaclib import *

def pre_build():
  run_cmd(['updpkgsums'])

def post_build():
  git_add_files('PKGBUILD')
  git_commit()
