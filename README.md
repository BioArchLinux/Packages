# Packages
Scripts of Lilac Repo, including PKGBUILD, lilac.yaml and lilac.py

## Aim

### Current goal

 - Migrate about 50 packages from aur_build to lilac.
 - Expansion of the users' awareness of this repository.
 - Include more popular packages for biologists.

### Todo

Now sponsors are still needed to promote the net speed of this repository. The funding will be used to buy faster a VPS service and a domain name. Now the repository is hosted in 1M VPS in Shenzhen, China and the domain is malacology.net. `bioarchlinux.org` or `bioarchlinux.net` should be bought and VPS is better to locate in Hongkong, China or Singapore considering the internet block and internet downloading speed.

Or some undergraduate students can hold a [National Students' Innovation Entrepreneurship Training Program](http://gjcxcy.bjtu.edu.cn/Index.aspx) in his/her university to win the funding for this project. [starsareintherose]( mailto:guoyizhang@malacology.net) can give help to the undergraduate, considering her previous national level NSIETP.

## Invovled

### Todo List

All the todo list for this repository is fixed at Projects. For contributing to new packages, see [packages todo list](https://github.com/BioArchLinux/Packages/projects/1), the in progress column shows the packages which are needed to be fixed, and the todo list column contains the packages which are needed to build and update to this repository. For fixing the bugs, see the [bug todo list](https://github.com/BioArchLinux/Packages/projects/2), the issues reporting bugs will be moved to the todo column of this project.

### Pull Request

For contributing to this repository, we would like to invite you as a member of this organization. About the details, see the [English contribution guide](./CONTRIBUTING_EN.md) or the [Chinese contribution guide](./CONTRIBUTING.md ).

### Discussions

When you meet some difficulty in writing `PKGBUILD`, `lilac.yaml` and so on. There are two web pages that you could find help, one is bbs.archlinux.org and another is our [discussions](https://github.com/BioArchLinux/Packages/discussions/categories/qa).

### Periodical Flags

When almost all packages are uploaded and fixed, the periodical flags would help this community work better. Some potential flags will be listed on the [discussions](https://github.com/BioArchLinux/Packages/discussions/categories/periodical-flags) and users can comment on it. After merging some comments, the team can publish [announcements](https://github.com/BioArchLinux/Packages/discussions/categories/announcements) on discussions to show the tasks plan.

### Bug Reports & Package Request

For bug reports, just posts some [issues](https://github.com/BioArchLinux/Packages/issues) with a detailed output log and give us detailed information of your working environment, we will fix it. It's almost the same as the request, give details about the packages information following the template.

## Contact

If you would like to join us, try to contact [starsareintherose](mailto:guoyizhang@malacology.net). Better to introduce what you want to do in our organization.

## Usage

Add the fllowing to the `/etc/pacman.conf`
```
[BioArchLinux]
Server = https://repo.malacology.net/
```
Then import GPG key
```
$ pacman-key --recv-keys B1F96021DB62254D
$ pacman-key --finger B1F96021DB62254D
$ pacman-key --lsign-key B1F96021DB62254D
```

## License and Code of Conduct

License is [GPL version 3](./LICENSE) and Code of Conduct is [here](./CODE_OF_CONDUCT.md).

## Acknowledgments

Thanks go to [lilydjwg](https://github.com/lilydjwg), the creator of lilac, for her help on lilac. It's grateful for all the users for using this repository and supporting BioArchLinux.
