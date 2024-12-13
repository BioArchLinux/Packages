<h3 align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/bioarchlinux.png" alt="BioArchLinux" width="150">
</p>
BioArchLinux</h3>

<h3 align="center">
<a href="https://bioarchlinux.org/packages">Packages</a> <a href="https://wiki.bioarchlinux.org">Wiki</a> <a href="https://matrix.to/#/#bioarchlinux:matrix.org">Matrix</a> <a href="https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio">Mirror</a> <a href="https://repo.bioarchlinux.org">Repo</a> <a href="https://build.bioarchlinux.org">Status</a>
</h3>

## Special sponsor

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/xtom.png"></a>
</div>

## Usage

### ArchLinux and ArchLinux based distributions

Add the following content to the `/etc/pacman.conf`
```
[bioarchlinux]
Server = https://repo.bioarchlinux.org/$arch
```
You can replace the `https://repo.bioarchlinux.org/$arch` with any mirror in [mirrorlist](https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio)

install GPG keyring to use bioarchlinux
```
# pacman-key --recv-keys B1F96021DB62254D
# pacman-key --finger B1F96021DB62254D
# pacman-key --lsign-key B1F96021DB62254D
```

update the database
```
# pacman -Syu
```

install any package
```
# pacman -S foo
```

### Windows

You can use [WSL](https://docs.microsoft.com/en-us/windows/wsl/install) to use our repo, see [here](https://github.com/BioArchLinux/wsl)

### Docker 
You can use [docker](https://hub.docker.com/r/bioarchlinux/bioarchlinux) to use our repo, see [here](https://github.com/BioArchLinux/docker)

### ISO
You can use ISO image via virtual machine software to use our repo, see [here](https://github.com/BioArchLinux/iso)

### non-root Linux users
You can use [junest](https://github.com/fsquillace/junest) BioArchLinux image to use our repo, see [here](https://github.com/BioArchLinux/junest-img)

## Citation

> Zhang G. Hu Y. Drobot V. Staal J. Yi Y. Elliot MG. 2022. BioArchLinux: bioinformatics community with Arch Linux. F1000Research 11: 809. https://doi.org/10.7490/f1000research.1119039.1

[Get Bibtex Here](https://raw.githubusercontent.com/BioArchLinux/Packages/master/BioArchLinux.bibtex)

## Submitting packages
Those wishing to submit 'PKGBUILD' for packages to repo please refer to [adding packages](/Documentation/adding%20packages.md) document 
