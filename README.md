<p align="center">
<img src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/bioarchlinux.png" alt="BioArchLinux" width="120">
</p>
<h1 align="center">BioArchLinux</h1>

## Special sponser

<div>
<a href="https://v.ps/" target="_blank"><img height="70px" src="https://raw.githubusercontent.com/BioArchLinux/Packages/master/logs/xtom.png"></a>
</div>

## Invovled

### Todo List

All the todo list for this repository is fixed at Projects. For contributing to new packages, see [packages todo list](https://github.com/BioArchLinux/Packages/projects/3), the in progress column shows the packages which are needed to be fixed, and the todo list column contains the packages which are needed to build and update to this repository. For fixing the bugs, see the [bug todo list](https://github.com/BioArchLinux/Packages/projects/2), the issues reporting bugs will be moved to the todo column of this project.

### Pull Request

For contributing to this repository, we would like to invite you as a member of this organization. About the details, see the [English contribution guide](./CONTRIBUTING_EN.md) or the [Chinese contribution guide](./CONTRIBUTING.md ).

### Discussions

When you meet some difficulty in writing `PKGBUILD`, `lilac.yaml` and so on. There are two web pages that you could find help, one is bbs.archlinux.org and another is our [discussions](https://github.com/BioArchLinux/Packages/discussions/categories/qa).

### Periodical Flags

When almost all packages are uploaded and fixed, the periodical flags would help this community work better. Some potential flags will be listed on the [discussions](https://github.com/BioArchLinux/Packages/discussions/categories/periodical-flags) and users can comment on it. After merging some comments, the team can publish [announcements](https://github.com/BioArchLinux/Packages/discussions/categories/announcements) on discussions to show the tasks plan.

### Mirror

```
rsync -rtlivH bioarchlinux.org::sync/ destdir 
```

### Bug Reports & Package Request

For bug reports, just posts some [issues](https://github.com/BioArchLinux/Packages/issues) with a detailed output log and give us detailed information of your working environment, we will fix it. It's almost the same as the request, give details about the packages information following the template.

## Contact

If you would like to join us, try to contact [starsareintherose](mailto:guoyizhang@malacology.net). Better to introduce what you want to do in our organization.

Or you can communicate at IRC channel #bioarchlinux at Libera Chat.

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
