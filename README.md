<h3 align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/bioarchlinux.png" alt="BioArchLinux" width="150">
</p>
BioArchLinux</h3>

## Special sponsor

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/xtom.png"></a>
</div>

## Usage

Add the fllowing content to the `/etc/pacman.conf`
```
[bioarchlinux]
Server = https://repo.bioarchlinux.org/$arch
```
You can repleace the `https://repo.bioarchlinux/$arch` with any mirror in [mirrorlist](https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio)

install GPG keyring to use bioarchlinux
```
sudo pacman -Sy && sudo pacman -S bioarchlinux-keyring
```

## Details

For details of [usage](https://github.com/BioArchLinux/Packages/wiki/Usage), [participation](https://github.com/BioArchLinux/Packages/wiki/Participation), [contribution](https://github.com/BioArchLinux/Packages/wiki/Contribution-to-the-repository), [maintainence contact](https://github.com/BioArchLinux/Packages/wiki/Become-maintainer) and [sponsorship](https://github.com/BioArchLinux/Packages/wiki/Sponsorship), please see our repository wiki page.

## License and Code of Conduct

License is [AGPL-3.0](./LICENSE) and Code of Conduct is [here](./CODE_OF_CONDUCT.md).

## Acknowledgments

Thanks go to [lilydjwg](https://github.com/lilydjwg), the creator of lilac, for her help on lilac. It's grateful for all the users for using this repository and supporting BioArchLinux.
