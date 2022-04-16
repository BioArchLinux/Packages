<h3 align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/bioarchlinux.png" alt="BioArchLinux" width="150">
</p>
BioArchLinux</h3>

## Special sponsor

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/xtom.png"></a>
</div>

## Contact

If you would like to join us, try to contact [starsareintherose](mailto:guoyizhang@malacology.net). Better to introduce what you want to do in our organization.

Or you can communicate at IRC channel #bioarchlinux at Libera Chat or Matrix #bioarchlinux:matrix.org.

## Usage

Add the fllowing to the `/etc/pacman.conf`
```
[BioArchLinux]
Server = https://repo.bioarchlinux.org/$arch
```
You can repleace the repo.bioarchlinux with any mirror in [mirrorlist](https://raw.githubusercontent.com/BioArchLinux/mirror/main/mirrorlist.bio)

Then import GPG key
```
$ pacman-key --recv-keys B1F96021DB62254D
$ pacman-key --finger B1F96021DB62254D
$ pacman-key --lsign-key B1F96021DB62254D
```

## License and Code of Conduct

License is [AGPL-3.0](./LICENSE) and Code of Conduct is [here](./CODE_OF_CONDUCT.md).

## Acknowledgments

Thanks go to [lilydjwg](https://github.com/lilydjwg), the creator of lilac, for her help on lilac. It's grateful for all the users for using this repository and supporting BioArchLinux.
