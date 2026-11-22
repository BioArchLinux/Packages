<h3 align="center">
<img src="logo/bioarchlinux.png" alt="BioArchLinux" width="150"><br>
BioArchLinux
</h3>

<h3 align="center">
<a href="https://bioarchlinux.org/packages">Packages</a> <a href="https://wiki.bioarchlinux.org">Wiki</a> <a href="https://matrix.to/#/#bioarchlinux:matrix.org">Matrix</a> <a href="https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio">Mirror</a> <a href="https://repo.bioarchlinux.org">Repo</a> <a href="https://build.bioarchlinux.org">Status</a>
</h3>

BioArchLinux is a community-driven repository of up-to-date, reproducible life-science software for Arch Linux and the distributions based on it.

## Special sponsor

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/xtom.png"></a>
</div>

## Usage

### ArchLinux and ArchLinux based distributions

1. Add the repository to `/etc/pacman.conf`:
```
[bioarchlinux]
Server = https://repo.bioarchlinux.org/$arch
```
You can replace `https://repo.bioarchlinux.org/$arch` with any server from the [mirror list](https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio).

2. Import and locally sign the repository's signing key:
```
# pacman-key --recv-keys B1F96021DB62254D
# pacman-key --finger B1F96021DB62254D
# pacman-key --lsign-key B1F96021DB62254D
```

3. Update the system:
```
# pacman -Syu
```

4. Install any package:
```
# pacman -S foo
```

### Windows

You can use the repository through [WSL](https://learn.microsoft.com/en-us/windows/wsl/install) with the [BioArchLinux WSL image](https://github.com/BioArchLinux/wsl).

### Docker 
You can use the [BioArchLinux Docker image](https://hub.docker.com/r/bioarchlinux/bioarchlinux), built from [BioArchLinux/docker](https://github.com/BioArchLinux/docker).

### ISO
You can run the [BioArchLinux ISO image](https://github.com/BioArchLinux/iso) in a virtual machine.

### non-root Linux users
You can use [JuNest](https://github.com/fsquillace/junest) with the [BioArchLinux JuNest image](https://github.com/BioArchLinux/junest-img).

## Citation

> Zhang G, Ristola P, Su H, Kumar B, Zhang B, Hu Y, Elliot MG, Drobot V, Zhu J, Staal J, Larralde M, Wang S, Yi Y, Yu H. 2025. BioArchLinux: community-driven fresh reproducible software repository for life sciences. Bioinformatics 41(3): btaf106. https://doi.org/10.1093/bioinformatics/btaf106
> 
> Zhang G, Hu Y, Drobot V, Staal J, Yi Y, Elliot MG. 2022. BioArchLinux: bioinformatics community with Arch Linux. Poster at BOSC 2022. F1000Research 11: 809. https://doi.org/10.7490/f1000research.1119039.1

[BibTeX](BioArchLinux.bibtex)

## Submitting packages
To add or fix a package, see the [contributing guide](CONTRIBUTING.md). Questions are welcome in the [Matrix room](https://matrix.to/#/#bioarchlinux:matrix.org).
