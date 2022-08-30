# Exabayes Readme 

ExaBayes is a software package for Bayesian phylogenetic tree
inference. It is particularly suitable for large-scale analyses on
computer clusters.

## Repository structure

This ExaBayes repository supports is now gitflow oriented.

It contains the following branches: 

* _devel:_ latest potentially unstable changes, bugfixes before they enter releases
* _master:_ a commit on the master branch (always tagged) is a specific version of ExaBayes
* _feature branches_: intended for development only 

## Documentation

Please find a detailed manual in manual/manual.pdf or
manual/manual.html.

An online version of the manual is available on the ExaBayes site on
the [Exelixis website][1].

Possibly, blog posts may appear on my [personal webpage][2].


## Installation

Preferably download, compile and install a released version (available
on the [Exelixis website][1]).

The slightly harder to way to install latest development version is to
run `bootstrap.sh` (you need to have installed autoconf and an
up-to-date version of autoconf-archive).

## Identifiability

All released versions correspond to a git-tag and thus allow you to
trace which code exactly was used when performing an analysis.

If you build from the devel-branch, ExaBayes specifies a `pre-X`
version in the header of the log (where X is the succeeding
version). Furthermore, ExaBayes prints the git commit id and git
commit date.

## Support

Since I have started working in a full-time job outside academia, my
capabilities to support ExaBayes are fairly limited, despite
intentions to keep the project running as well as possible. Please
keep nagging me in particular about bug fixes, until you obtain a
result that you see fit.

Consider posting in the [ExaBayes google group][3].

## License

Regarding licensing, please consider the file COPYING.

[1]: http://sco.h-its.org/exelixis/web/software/exabayes/index.html
[2]: http://aberer.io
[3]: https://groups.google.com/forum/#!forum/exabayes
