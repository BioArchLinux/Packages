# Contributing to BioArchLinux

Thanks for your interest in contributing packages to the repo. Before submitting packages, make sure your `PKGBUILD` is in line with the packaging guidelines in the [Arch Wiki](https://wiki.archlinux.org/title/PKGBUILD) and can be built in a [clean chroot](#building-in-a-clean-chroot). This document also assumes you have signed up for a GitHub account and set up SSH keys as outlined in the GitHub [documentation](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/about-ssh).

[lilac](https://github.com/archlinuxcn/lilac) builds and updates the packages here. The [Contribution](https://wiki.bioarchlinux.org/Contribution) page on our wiki gives a short overview of it.

## Workflow

1. Create a fork of the [BioArchLinux Packages](https://github.com/BioArchLinux/Packages) repository and clone it locally.
   ```
   git clone <ssh link to your forked repo>
   ```
2. Create a new branch for the package you wish to add.
   ```
   git checkout -b <pkgname>
   ```
3. Set the BioArchLinux repository as upstream.
   ```
   git remote add upstream https://github.com/BioArchLinux/Packages.git
   ```
4. Add or edit the package folder under `BioArchLinux/` (see [Package layout](#package-layout)).
5. Build the package in a clean chroot (see [Building in a clean chroot](#building-in-a-clean-chroot)).
6. Commit the changes (see [Commit messages](#commit-messages)).
   ```
   git add BioArchLinux/<pkgname>
   git commit -m "<pkgname>: init pkg"
   ```
7. Push the changes.
   ```
   git push -u origin <pkgname>
   ```
8. Open the URL in the output to create a pull request, and fill in the [pull request template](.github/PULL_REQUEST_TEMPLATE.md).
9. Wait for review and approval for the pull request to be merged.

## Package layout

Each package has its own folder, named after the package:

```
BioArchLinux/<pkgname>/
├── PKGBUILD
├── lilac.yaml
└── lilac.py      (optional)
```

## lilac.yaml

`lilac.yaml` tells lilac three things: who maintains the package, when to update or rebuild it, and what to build first. For example:

```yaml
build_prefix: extra-x86_64
maintainers:
- github: your-github-name
  email: you@example.org
repo_depends:
- r-matrixstats
update_on:
- source: rpkgs
  pkgname: BRISC
  repo: cran
  md5: true
- alias: r
```

lilac builds in a chroot. The chroot only has the Arch repositories, plus the packages from this repository that `lilac.yaml` lists. So:

- put every `[bioarchlinux]` package from `depends` under `repo_depends`;
- put every `[bioarchlinux]` package from `makedepends` and `checkdepends` under `repo_makedepends`.

If one is missing, the build fails with "target not found".

`update_on` takes [nvchecker](https://nvchecker.readthedocs.io/en/latest/usage.html) entries. The first entry checks for new versions. The other entries only trigger rebuilds. The [lilac.yaml documentation](https://archlinuxcn.github.io/lilac/) describes the other fields.

If the update steps are longer than a line or two, write them in a `lilac.py` as `pre_build()` and `post_build()`, instead of `pre_build_script` and `post_build_script` (see the [lilac API](https://lilac.readthedocs.io/en/latest/api.html)).

## Building in a clean chroot

Build with devtools before opening the pull request. If every dependency is in the Arch repositories, run this in the package folder:

```sh
extra-x86_64-build
```

If the package depends on something in `[bioarchlinux]`, you need a build target that includes our repository. You only need to set this up once. The chroot copies the pacman keyring from your system, so first import our signing key as shown in the [README](README.md). Then run:

```sh
sudo cp /usr/share/devtools/pacman.conf.d/extra.conf /usr/share/devtools/pacman.conf.d/bioarchlinux.conf
printf '\n[bioarchlinux]\nServer = https://repo.bioarchlinux.org/$arch\n' | sudo tee -a /usr/share/devtools/pacman.conf.d/bioarchlinux.conf
sudo ln -s /usr/bin/archbuild /usr/local/bin/bioarchlinux-x86_64-build
```

Then build with `bioarchlinux-x86_64-build`. If the package needs another new package from the same pull request, build that one first and install it into the chroot:

```sh
bioarchlinux-x86_64-build -- -I ../<dependency>/<dependency>-<version>-x86_64.pkg.tar.zst
```

To recreate the chroot from scratch, add `-c` before the `--`. Read the namcap output at the end of the build.

`pkgctl build` does not work here, because it only uses the official Arch repositories.

## Commit messages

One commit per package, with the package name as a prefix:

- `<pkgname>: init pkg` (or just `init`) for a new package
- `<pkgname>: update to <version>`
- `<pkgname>: fix build`, `<pkgname>: fix depends`, and so on for fixes
- `<pkgname>: rm pkg` to remove a package

Packages that depend on each other can go in one pull request. Use all their names in the title, like `r-gpgp r-gpvecchia: init pkgs`. Put the commit for each dependency before the package that needs it.

lilac usually updates versions itself (`<pkgname>: auto updated to <version>`), so you seldom need to do it by hand.

## R packages

Most packages here are R packages from CRAN and Bioconductor. They are named `r-<name in lower case>`. The easiest way to start is to copy a recent one, such as `BioArchLinux/r-brisc` for CRAN or `BioArchLinux/r-biocgenerics` for Bioconductor.

On every update, `lilac-extensions/lilac_r_utils.py` compares the `PKGBUILD` with the package's `DESCRIPTION`. Fill in the `PKGBUILD` like this:

| `PKGBUILD` | From `DESCRIPTION` |
|---|---|
| `pkgdesc` | `Title`, without a leading package name or a trailing full stop |
| `arch` | `x86_64` if `NeedsCompilation: yes`, otherwise `any` |
| `license` | `License` in SPDX form, from `license_map` in `lilac_r_utils.py` (`GPL (>= 2)` is `GPL-2.0-or-later`, `MIT + file LICENSE` is `MIT`) |
| `depends` | `Depends` and `Imports` |
| `makedepends` | `LinkingTo`, minus what is already in `depends`, plus `gcc-fortran` if there are Fortran sources |
| `optdepends` | `Suggests` |
| `md5sums` | the md5 sum CRAN or Bioconductor publishes |

Do not list the packages that come with R, such as `methods`, `stats`, `Matrix` and `MASS`. The `r` package already provides them, and the check fails if they are listed.

Use the `update_on` entry from the example above, with `repo: cran` or `repo: bioc`, and call `r_pre_build` from `lilac.py`:

```python
#!/usr/bin/env python3
from lilaclib import *

import os
import sys
sys.path.append(os.path.normpath(f'{__file__}/../../../lilac-extensions'))
from lilac_r_utils import r_pre_build

def pre_build():
    r_pre_build(_G)

def post_build():
    git_pkgbuild_commit()
```

`r_pre_build` updates the version and checksums, and then runs the check.

If the license is not in `license_map`, write the `license` array yourself. Then pass the license exactly as the `DESCRIPTION` writes it, for example `r_pre_build(_G, expect_license = "GPL (>= 2) | file LICENSE")`. Other options, like `expect_systemrequirements`, are in `CheckConfig` in `lilac_r_utils.py`.

## Syncing to the AUR

Many packages here are also on the AUR. lilac keeps them up to date there as BioArchLinuxBot. To sync your package:

1. Make sure the package exists on the AUR.
2. Add BioArchLinuxBot as a co-maintainer there.
3. Call `update_aur_repo()` in `post_build`, after `git_pkgbuild_commit()`.

Rebuilds that only change `pkgrel` are not pushed to the AUR.
