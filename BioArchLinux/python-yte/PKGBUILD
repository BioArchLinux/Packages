# Maintainer: Ivan Batrakov <blackfan321 at disroot dot org>
# Contributor: Philipp A. <flying-sheep@web.de>

_name=yte
pkgname=python-$_name
pkgver=1.5.5
pkgrel=1
pkgdesc='YAML template engine with Python expressions'
arch=(any)
url="https://github.com/koesterlab/$_name"
license=(MIT)
depends=(python-dpath python-plac python-pyyaml)
makedepends=(python-poetry-core python-build python-installer python-wheel)
source=("https://files.pythonhosted.org/packages/source/${_name::1}/$_name/$_name-$pkgver.tar.gz")
sha256sums=('2c49831859f3216f313a17688900690872e05f8fbe77cb5d151bdb896357d57e')

build() {
	cd "$_name-$pkgver"
	python -m build --wheel --no-isolation
}

package() {
	cd "$_name-$pkgver"
	python -m installer --destdir="$pkgdir" dist/*.whl
}
