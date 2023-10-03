#!/usr/bin/env python3
import requests
import fileinput
from lilaclib import *

def get_latest_tag(owner, repo):
    url = f"https://api.github.com/repos/{owner}/{repo}/tags"
    response = requests.get(url)
    response.raise_for_status()
    tags = response.json()
    latest_tag = tags[0]["name"].lstrip("v")
    return latest_tag

def pre_build():    
    latest_tag = get_latest_tag("pcingola", "SnpSift")
    for line in edit_file('PKGBUILD'):
        if line.startswith('_pkgver2='):
            current_tag = line.split("=")[1].strip()
            if latest_tag > current_tag:
                line = f'_pkgver2={latest_tag}'
        print(line)

    update_pkgver_and_pkgrel(_G.newver.lstrip('v'))