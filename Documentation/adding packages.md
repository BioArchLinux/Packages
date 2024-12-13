# How to add packages to repo
Thanks for you interest in contributing packages to repo. Before submitting packages make sure your `PKGBUILD` is in line with packaging guidelines as outlined in [Arch Wiki](https://wiki.archlinux.org/title/PKGBUILD) and can be built in [clean chroot](https://wiki.archlinux.org/title/DeveloperWiki:Building_in_a_clean_chroot#Classic_way). This document also assumes you have signed up for github account and has setup ssh keys as outlined in github [documentation](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/about-ssh)
1. create a fork of the [Bioarchlinux Packages](https://github.com/BioArchLinux/Packages) repo and clone it locally.
```
git clone <ssh link to your forked repo>
```
2. Create a new branch for the package you wish to add.
```
git checkout -b <pkgname>
```
3. Set bioarchlinux repo as upstream for the branch.
```
git remote add upstream https://github.com/BioArchLinux/Packages.git
```
4. make the required changes such as adding respective package folder containing PKGBUILD and lilac.yaml files into the **Bioarchlinux** folder. The folder structure should look like this,
```
BioArchLinux/<pkgname>/
                    ├── lilac.yaml
                    └── PKGBUILD
```
5. commit the changes by
```
git add <changed file name goes here>
git commit -m "pkgname: adding version *xxx.yyy*"
```
6. push the changes by,
```
git push -u origin <pkgname>
```
7. [ctrl]+left click the output URL to open pull request in a browser. 
8. Wait for review and approval for PR to be merged. 
