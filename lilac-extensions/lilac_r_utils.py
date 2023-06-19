from lilac2.const import PACMAN_DB_DIR
from lilaclib import edit_file, run_protected
import pyalpm
import tarfile
from types import SimpleNamespace

def r_update_pkgver_and_pkgrel(newver: str):
    """
    Update _pkgver and pkgrel used in R packages.

    The pkgver variable (without underscore) should be defined as `${_pkgver//[:-]/.}`.
    """
    ver_prefix = "_pkgver="
    rel_prefix = "pkgrel="
    oldver = None
    oldrel = None
    for line in edit_file("PKGBUILD"):
        if line.startswith(ver_prefix):
            if oldver is not None:
                raise Exception("_pkgver is defined twice")
            oldver = line[len(ver_prefix):]
            line = f"{ver_prefix}{newver}"
        elif line.startswith(rel_prefix):
            if oldver is None:
                raise Exception("pkgrel is defined before _pkgver")
            if oldrel is not None:
                raise Exception("pkgrel is defined twice")
            oldrel = int(line[len(rel_prefix):])
            newrel = oldrel + 1 if oldver == newver else 1
            line = f"{rel_prefix}{newrel}"
        print(line)
    if oldrel is None:
        raise Exception("pkgrel is not defined")

class Description:
    """Metadata from the DESCRIPTION file, package names are converted to Arch format"""

    def __init__(self, tar: tarfile.TarFile, name: str):
        self.desc = self._parse_description(tar, name)

        self.title = self.desc["Title"]
        self.depends = self._parse_deps("Depends")
        self.imports = self._parse_deps("Imports")
        self.linkingto = self._parse_deps("LinkingTo")
        self.suggests = self._parse_deps("Suggests")
        self.systemrequirements = self.desc.get("SystemRequirements", None)
        self.license = self.desc["License"]
        nc = self.desc["NeedsCompilation"]
        if nc != "yes" and nc != "no":
            raise Exception(f"Invalid DESCRIPTION file: NeedsCompilation: {nc}")
        self.needscompilation = nc == "yes"

    def _parse_description(self, tar: tarfile.TarFile, name: str) -> dict:
        """Parse the DESCRIPTION file from the source archive and return it as a dict."""
        space = b" "[0]
        tab = b"\t"[0]

        # Avoid decoding any strings before the encoding used in the DESCRIPTION file is known
        # file format specification: https://cran.r-project.org/doc/manuals/R-exts.html#The-DESCRIPTION-file
        rawdata = dict()
        with tar.extractfile(f"{name}/DESCRIPTION") as desc:
            value = None
            for line in desc:
                c = line[0]
                if c == space or c == tab:
                    if value is None:
                        raise Exception("Invalid DESCRIPTION")
                    value.append(space)
                    value.extend(line.strip())
                else:
                    i = line.find(b": ")
                    if i == -1:
                        error = line.decode(errors = "replace")
                        raise Exception("Invalid line in DESCRIPTION: '{error}'")
                    field = line[:i]
                    value = bytearray(line[i+2:-1])
                    rawdata[field] = value

        enc_key = b"Encoding"
        if enc_key in rawdata:
            encoding = rawdata[enc_key].decode()
            if encoding not in ["UTF-8", "latin1", "latin2"]:
                raise Exception(f"Invalid encoding: {encoding}")
        else:
            encoding = "UTF-8"

        return {field.decode(encoding): value.decode(encoding) for field, value in rawdata.items()}

    def _r_name_to_arch(self, r_pkg_name: str) -> str:
        """Converts R package name to the corresponding Arch package"""
        return f"r-{r_pkg_name.lower()}"

    def _parse_deps(self, field: str) -> list:
        """Parse a field that contains a list of dependencies"""
        if field not in self.desc:
            return []
        ret = []
        for dep in self.desc[field].split(", "):
            i = dep.find(" (")
            if i != -1:
                dep = dep[:i]
            if dep != "R":
                ret.append(self._r_name_to_arch(dep))
        return ret

class Pkgbuild:
    """PKGBUILD variable values"""

    __variables = [
        "_pkgname",
        "pkgdesc",
    ]
    __arrays = [
        "arch",
        "license",
        "depends",
        "makedepends",
        "checkdepends",
        "optdepends",
        "md5sums",
    ]

    def __init__(self):
        cmd = ["/bin/bash", "-c", "source PKGBUILD && declare -p"]
        output = run_protected(cmd, silent = True)
        # assume that variable values never contain newlines
        for line in output.splitlines():
            self._parse_line(line)
        for var in Pkgbuild.__variables:
            if not hasattr(self, var):
                setattr(self, var, None)
        for var in Pkgbuild.__arrays:
            if not hasattr(self, var):
                setattr(self, var, [])

    def _parse_line(self, line: str):
        variable_prefix = "declare -- "
        array_prefix = "declare -a "
        if line.startswith(variable_prefix):
            split = line[len(variable_prefix):].split("=", 1)
            if len(split) != 2:
                return
            variable, value = split
            if variable in Pkgbuild.__variables:
                setattr(self, variable, self._parse_value(value))
        elif line.startswith(array_prefix):
            split = line[len(array_prefix):].split("=", 1)
            if len(split) != 2:
                return
            variable, values = split
            if variable in Pkgbuild.__arrays:
                setattr(self, variable, self._parse_array(values))

    def _parse_array(self, array: str) -> list:
        if array[0] != '(' or array[-1] != ')':
            raise Exception("Fatal error")
        array = array[1:-1]
        values = []
        start_index = 0
        while True:
            start_index = array.find('"', start_index)
            if start_index == -1:
                break
            end_index = None
            i = start_index + 1
            while i < len(array):
                if array[i] == '\\':
                    i += 1
                elif array[i] == '"':
                    end_index = i + 1
                    break
                i += 1
            if end_index is None:
                raise Exception("Array value is not closed")
            values.append(self._parse_value(array[start_index:end_index]))
            start_index = end_index
        return values

    def _parse_value(self, value: str) -> str:
        if value[0] != '"' or value[-1] != '"':
            raise Exception("Fatal error")
        return value[1:-1].replace('\\"', '"').replace('\\\\', '\\')

# maps the license field in the DESCRIPTION file to a PKGBUILD license value
license_map = {
    "Apache License (== 2.0)": "Apache",
    "Apache License 2.0": "Apache",
    "BSD_2_clause + file LICENSE": "BSD",
    "BSD_3_clause + file LICENSE": "BSD",
    "BSL-1.0": "Boost",
    "EPL": "EPL",
    "GPL": "GPL",
    "GPL (>= 2)": "GPL",
    "GPL (>= 3)": "GPL3",
    "GPL-2": "GPL2",
    "GPL-2 | GPL-3": "GPL",
    "GPL-3": "GPL3",
    "LGPL (>= 2)": "LGPL",
    "LGPL (>= 2.1)": "LGPL",
    "LGPL-2": "LGPL2.1",
    "MIT + file LICENSE": "MIT",
}

def get_default_r_pkgs() -> set:
    """Get the set of R packages included in the R distribution itself"""
    provides = pyalpm.Handle("/", str(PACMAN_DB_DIR)).register_syncdb("extra", 0).get_pkg("r").provides
    return { pr.split("=", 1)[0] for pr in provides }

class CheckFailed(Exception):
    def __init__(self, msg: str):
        super().__init__(msg)
        self.msg = msg

class CheckConfig:
    def __init__(self,
        expect_license: str = None,
        expect_systemrequirements: str = None,
        ignore_fortran_files: bool = False,
    ):
        self.expect_license = expect_license
        self.expect_systemrequirements = expect_systemrequirements
        self.ignore_fortran_files = ignore_fortran_files

def check_default_pkgs(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    errors = set()
    for dep in pkg.depends + pkg.makedepends + pkg.checkdepends + pkg.optdepends:
        if dep in cfg.default_r_pkgs:
            errors.add(dep)
    if len(errors) > 0:
        errors = ", ".join(errors)
        raise CheckFailed(f"Dependency is included in the R distribution: {errors}")

def check_depends(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    implicit_r_dep = explicit_r_dep = False
    errors = []
    for dep in pkg.depends:
        if dep.startswith("r-"):
            if (dep not in desc.depends) and (dep not in desc.imports):
                errors.append(f"Unnecessary dependency: {dep}")
            else:
                implicit_r_dep = True
        elif dep == "r":
            explicit_r_dep = True

    not_missing = True
    for dep in desc.depends + desc.imports:
        if (dep not in cfg.default_r_pkgs) and (dep not in pkg.depends):
            not_missing = False
            errors.append(f"Missing dependency: {dep}")

    if implicit_r_dep and explicit_r_dep:
        errors.append("Unnecessary dependency: r")
    elif not_missing and not (implicit_r_dep or explicit_r_dep):
        errors.append("Missing dependency: r")

    if len(errors) > 0:
        raise CheckFailed('\n'.join(errors))

def check_makedepends(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    errors = []
    for dep in pkg.makedepends:
        if dep in pkg.depends or (dep.startswith("r-") and dep not in desc.linkingto):
            errors.append(f"Unnecessary make dependency: {dep}")

    for dep in desc.linkingto:
        if (dep not in cfg.default_r_pkgs) and (dep not in desc.depends) and (dep not in desc.imports) and (dep not in pkg.makedepends):
            errors.append(f"Missing make dependency: {dep}")

    if len(errors) > 0:
        raise CheckFailed('\n'.join(errors))

def check_optdepends(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    errors = []
    for dep in pkg.optdepends:
        if dep in pkg.depends or (dep.startswith("r-") and dep not in desc.suggests):
            errors.append(f"Unnecessary optional dependency: {dep}")

    for dep in desc.suggests:
        if (dep not in cfg.default_r_pkgs) and (dep not in pkg.optdepends):
            errors.append(f"Missing optional dependency: {dep}")

    if len(errors) > 0:
        raise CheckFailed('\n'.join(errors))

def check_fortran(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    fortran_files = False
    for name in cfg.tar.getnames():
        if name.endswith(".f") or name.endswith(".f90") or name.endswith(".for"):
            fortran_files = True
            break
    fortran_dep = "gcc-fortran" in pkg.makedepends

    if fortran_files and not fortran_dep:
        if not cfg.ignore_fortran_files:
            raise CheckFailed("Missing make dependency: gcc-fortran")
    elif not fortran_files and fortran_dep:
        raise CheckFailed("Unnecessary make dependency: gcc-fortran")
    elif cfg.ignore_fortran_files and not fortran_files:
        raise CheckFailed("Unnecessary config 'ignore_fortran_files'")

def check_systemrequirements(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    if cfg.expect_systemrequirements != desc.systemrequirements:
        raise CheckFailed(f"SystemRequirements have changed: {desc.systemrequirements}")

def check_license(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    if cfg.expect_license is not None:
        if cfg.expect_license != desc.license:
            raise CheckFailed(f"License in the DESCRIPTION has changed: {desc.license}")
    elif desc.license in license_map:
        expected = license_map[desc.license]
        if pkg.license != [expected]:
            raise CheckFailed(f"Wrong license, expected {expected}")
    else:
        raise CheckFailed(f"Unknown license: {desc.license}. Consider setting CheckConfig.expect_license")

def check_pkgdesc(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    if pkg.pkgdesc != desc.title:
        raise CheckFailed(f"Wrong pkgdesc, expected '{desc.title}'")

def check_arch(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    expected = "x86_64" if desc.needscompilation else "any"
    if pkg.arch != [expected]:
        raise CheckFailed(f"Wrong arch, expected {expected}")

def check_md5sum(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    if pkg.md5sums[0] != cfg.md5sum:
        raise CheckFailed(f"Wrong md5sum, expected {cfg.md5sum}")

all_checks = [
    check_default_pkgs,
    check_depends,
    check_makedepends,
    check_optdepends,
    check_fortran,
    check_systemrequirements,
    check_license,
    check_pkgdesc,
    check_arch,
    check_md5sum,
]

def r_check_pkgbuild(newver: str, cfg: CheckConfig):
    pkgbuild = Pkgbuild()
    cfg.default_r_pkgs = get_default_r_pkgs()
    errors = []
    with tarfile.open(f"{pkgbuild._pkgname}_{newver}.tar.gz", "r") as tar:
        description = Description(tar, pkgbuild._pkgname)
        cfg.tar = tar

        for check in all_checks:
            try:
                check(pkgbuild, description, cfg)
            except CheckFailed as e:
                errors.append(e.msg)
    if len(errors) > 0:
        errors = '\n'.join(errors)
        raise CheckFailed(f"Check failed:\n{errors}")

def r_pre_build(_G: SimpleNamespace, **kwargs):
    cfg = CheckConfig(**kwargs)
    newver, md5sum = _G.newver.rsplit("#", 1)
    cfg.md5sum = md5sum

    r_update_pkgver_and_pkgrel(newver)
    run_protected(["updpkgsums"])
    r_check_pkgbuild(newver, cfg)
