<h3 align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/bioarchlinux.png" alt="BioArchLinux" width="150">
</p>
BioArchLinux</h3>

<h3 align="center">
<a href="https://bioarchlinux.org/packages">Packages</a> <a href="https://wiki.bioarchlinux.org">Wiki</a> <a href="https://matrix.to/#/#bioarchlinux:matrix.org">Matrix</a> <a href="https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio">Mirror</a> <a href="https://repo.bioarchlinux.org">Repo</a> <a href="https://build.bioarchlinux.org">Status</a>
</h3>

## Special sponsor

<a href="https://v.ps/" target="_blank">
  <img align="middle" height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/xtom.png" alt="Logo 1">
</a>
<a href="https://maibloom.github.io/" target="_blank">
  <img align="middle" height="155px" src="https://github.com/user-attachments/assets/e7cdc81c-4328-4780-9bb7-f59ab1ae12c7" alt="Logo 2">
</a>

## Usage

### OmniPkg package manager

OmniPkg lets you add the BioArchLinux mirror to your system quickly.

- Run the following command to install OmniPkg:
```
curl -fsSL https://raw.githubusercontent.com/maibloom/omnipkg-app/main/indep-build.sh | bash
```

- After installing OmniPkg, add the BioArchLinux mirror with this command:
```
sudo omnipkg put install bioarchlinux-mirror
```

- **Alternatively**, you can combine both steps into a single command:
```
curl -fsSL https://raw.githubusercontent.com/maibloom/omnipkg-app/main/indep-build.sh | bash && sudo omnipkg put install bioarchlinux-mirror
```

### ArchLinux and ArchLinux based distributions

- Add the following content to the `/etc/pacman.conf`
```
[bioarchlinux]
Server = https://repo.bioarchlinux.org/$arch
```
> [!NOTE]
> You can replace the `https://repo.bioarchlinux.org/$arch` with any mirror in [mirrorlist](https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio)

- install GPG keyring to use bioarchlinux
```
# pacman-key --recv-keys B1F96021DB62254D
# pacman-key --finger B1F96021DB62254D
# pacman-key --lsign-key B1F96021DB62254D
```

- update the database
```
# pacman -Syu
```

- install any package
```
# pacman -S foo
```

### Mai Bloom OS

Mai Bloom OS comes with the BioArchLinux mirror preinstalled.

### Windows

You can use [WSL](https://docs.microsoft.com/en-us/windows/wsl/install) to use our repo, see [here](https://github.com/BioArchLinux/wsl)

### Docker 
You can use [docker](https://hub.docker.com/r/bioarchlinux/bioarchlinux) to use our repo, see [here](https://github.com/BioArchLinux/docker)

### ISO
You can use ISO image via virtual machine software to use our repo, see [here](https://github.com/BioArchLinux/iso)

### non-root Linux users
You can use [junest](https://github.com/fsquillace/junest) BioArchLinux image to use our repo, see [here](https://github.com/BioArchLinux/junest-img)

## Citation

> Zhang G, Ristola P, Su H, Kumar B, Zhang B, Hu Y, Elliot MG, Drobot V, Zhu J, Staal J, Larralde M, Wang S, Yi Y, Yu H. 2025. BioArchLinux: community-driven fresh reproducible software repository for life science. Bioinformatics: btaf106. https://doi.org/10.1093/bioinformatics/btaf106
> 
> Zhang G, Hu Y, Drobot V, Staal J, Yi Y, Elliot MG. 2022. BioArchLinux: bioinformatics community with Arch Linux. F1000Research 11: 809. https://doi.org/10.7490/f1000research.1119039.1

[Get Bibtex Here](https://raw.githubusercontent.com/BioArchLinux/Packages/master/BioArchLinux.bibtex)

## Submitting packages
Those wishing to submit 'PKGBUILD' for packages to repo please refer to [adding packages](/Documentation/adding%20packages.md) document 
