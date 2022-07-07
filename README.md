<h3 align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/bioarchlinux.png" alt="BioArchLinux" width="150">
</p>
BioArchLinux</h3>

## Special sponsor

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logo/xtom.png"></a>
</div>

## Usage

Add the fllowing content to the `/etc/pacman.conf`
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
# pacman -Syy
```

install any package
```
# pacman -S foo
```
Besides, you are suggested to install `r-bspm`,
```
# pacman -S r-bspm
```
and add the following line to your `~/.Rprofile` so that `pacman` will take over `install.packages`.
```R
bspm::enable()
```
Below is an example.
```R
r$> install.packages("cssam")
[sudo] password for user:
Install system packages as root...
:: Synchronizing package databases...
 core
 extra
 community
 multilib
 archlinuxcn
 arch4edu
 bioarchlinux
:: Retrieving packages...
 r-cssam-1.2.4-1-any
:: Processing package changes...
(1/1) r-cssam
```
## Request a package
If you find some package useful but not in our repository, (for example even if you used `r-bspm` it still installed a package from CRAN/Bioconductor),
you can open an issue and tell us.

## Details

For details of [usage](https://github.com/BioArchLinux/Packages/wiki/Usage), [participation](https://github.com/BioArchLinux/Packages/wiki/Participation), [contribution](https://github.com/BioArchLinux/Packages/wiki/Contribution-to-the-repository), [maintainence contact](https://github.com/BioArchLinux/Packages/wiki/Become-maintainer) and [sponsorship](https://github.com/BioArchLinux/Packages/wiki/Sponsorship), please see our repository wiki page.

## License and Code of Conduct

License is [AGPL-3.0](./LICENSE) and Code of Conduct is [here](./CODE_OF_CONDUCT.md).

## Acknowledgments

Thanks go to [lilydjwg](https://github.com/lilydjwg), the creator of lilac, for her help on lilac. It's grateful for all the users for using this repository and supporting BioArchLinux.
