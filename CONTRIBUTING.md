# Contributing to BioArchLinux

This repository holds the build recipes for every package in the `[bioarchlinux]` repository. [lilac](https://github.com/archlinuxcn/lilac) builds the packages from here, checks for new upstream versions, and updates the packages automatically.

Everyone taking part is expected to follow the [Code of Conduct](CODE_OF_CONDUCT.md).

## Reporting a problem or requesting a package

Use the issue templates, one package per issue:

- [Packages Bug](https://github.com/BioArchLinux/Packages/issues/new?template=packages-bug-.md) for a package that fails to build, install, or run. Include the log and your package version.
- [Package Request](https://github.com/BioArchLinux/Packages/issues/new?template=package-request.md) for software you would like to see packaged. Requests for widely used and cited software are considered first.

The build status of every package is at [build.bioarchlinux.org](https://build.bioarchlinux.org).

## Asking questions

Ask in [GitHub Discussions](https://github.com/BioArchLinux/Packages/discussions) or in the [Matrix room](https://matrix.to/#/#bioarchlinux:matrix.org).

## Adding or fixing a package

Packages follow the Arch packaging rules, so read the ArchWiki pages on [PKGBUILD](https://wiki.archlinux.org/title/PKGBUILD) and the [Arch package guidelines](https://wiki.archlinux.org/title/Arch_package_guidelines) first. The [Contribution](https://wiki.bioarchlinux.org/Contribution) page on our wiki gives a short overview of lilac.

### Workflow

1. Fork this repository and clone your fork.
2. Create a branch for your change.
3. Add or edit the package folder under `BioArchLinux/` (see below).
4. Build the package in a clean chroot.
5. Commit, push to your fork, and open a pull request, filling in the [pull request template](.github/PULL_REQUEST_TEMPLATE.md).

### Package layout

Each package has its own folder, named after the package:

```
BioArchLinux/<pkgname>/
├── PKGBUILD
├── lilac.yaml
└── lilac.py      (optional)
```

### lilac.yaml

`lilac.yaml` tells lilac who maintains the package, when to update or rebuild it, and what to build first. A typical one:

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

- **`maintainers`** receive lilac's email notifications about the package. Give your GitHub name, and an email address if the one on your GitHub profile is not public.
- **`update_on`** is a list of [nvchecker](https://nvchecker.readthedocs.io/en/latest/usage.html) entries. The first one checks for new versions and the others only trigger rebuilds. Common sources here are `rpkgs` (CRAN and Bioconductor), `github`, `pypi`, `regex`, and `alpm` or `alpmfiles` for rebuilding against a package in the Arch repositories. `alias: r` and `alias: python` rebuild the package when R or Python moves to a new minor version.
- **`repo_depends`** lists the packages from this repository in `depends`, and **`repo_makedepends`** those in `makedepends` or `checkdepends`. lilac builds them first and installs them into the build chroot.
- **`pre_build_script`** and **`post_build_script`** hold the bodies of the `pre_build` and `post_build` functions, if you don't use a `lilac.py`. Almost every `post_build_script` calls `git_pkgbuild_commit()`, which commits the updated `PKGBUILD` back to this repository.

The full reference is the [lilac.yaml documentation](https://archlinuxcn.github.io/lilac/).

### lilac.py

`lilac.py` does the same as the two scripts, with Python functions `pre_build()` and `post_build()`, and has the [lilac API](https://lilac.readthedocs.io/en/latest/api.html) available. Use it when the update steps are more than a line or two.

### Building in a clean chroot

Build with devtools before opening the pull request. If every dependency is in the Arch repositories, run this in the package folder:

```sh
extra-x86_64-build
```

If the package depends on something in `[bioarchlinux]`, you need a build target that includes our repository, which takes one-time setup. The chroot copies your system's pacman keyring, so import our signing key first, as in the [README](README.md).

```sh
sudo cp /usr/share/devtools/pacman.conf.d/extra.conf /usr/share/devtools/pacman.conf.d/bioarchlinux.conf
printf '\n[bioarchlinux]\nServer = https://repo.bioarchlinux.org/$arch\n' | sudo tee -a /usr/share/devtools/pacman.conf.d/bioarchlinux.conf
sudo ln -s /usr/bin/archbuild /usr/local/bin/bioarchlinux-x86_64-build
```

Then build with `bioarchlinux-x86_64-build`. To build a package that depends on another new package from the same pull request, build that one first and install it into the chroot:

```sh
bioarchlinux-x86_64-build -- -I ../<dependency>/<dependency>-<version>-x86_64.pkg.tar.zst
```

Add `-c` before the `--` to recreate the chroot from scratch. Read the namcap output at the end of the build. `pkgctl build` does not work here, since it only builds against the official Arch repositories.

### Commit messages

Use one commit per package, with the package name as a prefix:

- `<pkgname>: init pkg` (or just `init`) for a new package
- `<pkgname>: update to <version>`
- `<pkgname>: fix build`, `<pkgname>: fix depends`, and the like for fixes
- `<pkgname>: rm pkg` to remove a package

Packages that depend on each other can go in one pull request, titled with all their names: `r-gpgp r-gpvecchia: init pkgs`. Put each dependency's commit before the package that needs it.

Version updates are normally made by lilac itself, as `<pkgname>: auto updated to <version>`, so you rarely need to update a version by hand.

## R packages

Most packages here are R packages from CRAN and Bioconductor, named `r-<name in lower case>`. Copy a recent one, such as `BioArchLinux/r-brisc` for CRAN or `BioArchLinux/r-biocgenerics` for Bioconductor, and adapt it.

### PKGBUILD

`lilac-extensions/lilac_r_utils.py` checks the `PKGBUILD` against the package's `DESCRIPTION` file on every update, so write it to pass that check:

- **`pkgdesc`** is the `Title`, without a leading package name and without a trailing full stop.
- **`arch`** is `x86_64` when the package has `NeedsCompilation: yes`, otherwise `any`.
- **`license`** is the SPDX form of the `License` field, from `license_map` in `lilac_r_utils.py`. For example, `GPL (>= 2)` is `GPL-2.0-or-later` and `MIT + file LICENSE` is `MIT`.
- **`depends`** holds every package in `Depends` and `Imports`, **`makedepends`** the `LinkingTo` packages not already in `depends`, and **`optdepends`** every package in `Suggests`. Add `gcc-fortran` to `makedepends` if the package has Fortran sources.
- **Leave out the packages that come with R itself**, such as `methods`, `stats`, `Matrix`, and `MASS`. The `r` package provides them, and the check rejects them.
- **`md5sums`** is checked against the md5 sum CRAN or Bioconductor publishes for the tarball.

### lilac.py

Use the `update_on` entry from the example above, with `repo: cran` or `repo: bioc`, and `md5: true`. Then call `r_pre_build` from `lilac.py`:

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
    update_aur_repo()
```

`r_pre_build` updates `_pkgver`, `pkgrel`, and the checksums, then runs the check. If the `License` field is not in `license_map`, pass the field exactly as written in the `DESCRIPTION`, for example `r_pre_build(_G, expect_license = "GPL (>= 2) | file LICENSE")`, and write the `license` array yourself. `CheckConfig` in `lilac_r_utils.py` lists the other options, such as `expect_systemrequirements` for packages with a `SystemRequirements` field.

> [!WARNING]
> When the check finds a dependency error, `r_pre_build` rewrites the `r-*` entries of `depends`, `makedepends`, and `optdepends` itself, and rewrites `lilac.yaml` to match. `repo_depends` becomes the `r-*` entries of `depends` and `repo_makedepends` those of `makedepends`, and either is removed when it would be empty. Entries needed only by `checkdepends`, other packages from this repository, and comments are lost in the rewrite, so check `lilac.yaml` after an automatic fix.

## Syncing to the AUR

Many packages here are also on the AUR. `update_aur_repo()` in `post_build` pushes each update there as the BioArchLinuxBot account ([its packages](https://aur.archlinux.org/packages?SeB=m&K=BioArchLinuxBot)). This only works if the AUR package already exists and BioArchLinuxBot is its maintainer or a co-maintainer, so add BioArchLinuxBot as a co-maintainer on the AUR before adding the call. `lilac.yaml` and `lilac.py` are not copied to the AUR, and a rebuild that only changes `pkgrel` is not pushed.

## Removing packages

Packages removed from CRAN or Bioconductor are deleted here once nothing in this repository or on the AUR depends on them. To propose a removal, open a [Maintainers' Bug](https://github.com/BioArchLinux/Packages/issues/new?template=maintainers--bug.md) issue listing the packages, as in #197 and #205. A maintainer sends the AUR deletion requests from the BioArchLinuxBot account. A package that others on the list depend on is removed in a later batch, after those packages. The repository side is a `<pkgname>: rm pkg` commit that deletes the folder.
