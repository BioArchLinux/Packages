from lilac2.const import PACMAN_DB_DIR
from lilaclib import edit_file, run_cmd, run_protected
import os
import pyalpm
import re
import shlex
import tarfile
import yaml
from types import SimpleNamespace

def r_update_pkgver_and_pkgrel(newver: str):
    """
    Update _pkgver and pkgrel used in R packages.

    The pkgver variable (without underscore) should be defined as `${_pkgver//-/.}`.
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

def _r_name_to_arch(r_pkg_name: str) -> str:
    """Converts R package name to the corresponding Arch package"""
    return f"r-{r_pkg_name.lower()}"

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
        nc = self.desc.get("NeedsCompilation", None)
        if nc is not None and nc != "yes" and nc != "no":
            raise Exception(f"Invalid DESCRIPTION file: NeedsCompilation: {nc}")
        self.needscompilation = "" if nc is None else nc == "yes"

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
                    if len(value) > 0:
                        value.append(space)
                    value.extend(line.strip())
                else:
                    i = line.find(b":")
                    if i == -1:
                        error = line.decode(errors = "replace")
                        raise Exception(f"Invalid line in DESCRIPTION: '{error}'")
                    field = line[:i]
                    value = bytearray(line[i+1:-1].strip())
                    rawdata[field] = value

        enc_key = b"Encoding"
        if enc_key in rawdata:
            encoding = rawdata[enc_key].decode()
            if encoding not in ["UTF-8", "latin1", "latin2"]:
                raise Exception(f"Invalid encoding: {encoding}")
        else:
            encoding = "UTF-8"

        return {field.decode(encoding): value.decode(encoding) for field, value in rawdata.items()}

    def _parse_deps(self, field: str) -> list:
        """Parse a field that contains a list of dependencies"""
        if field not in self.desc:
            return []
        ret = []
        for dep in self.desc[field].split(","):
            i = dep.find("(")
            if i != -1:
                dep = dep[:i]
            dep = dep.strip()
            if dep != "R" and len(dep) > 0:
                ret.append(_r_name_to_arch(dep))
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
        return value[1:-1].replace('\\$', '$').replace('\\`', '`').replace('\\"', '"').replace('\\\\', '\\')

# maps the license field in the DESCRIPTION file to a PKGBUILD license value
license_map = {
    "AGPL-3": "AGPL-3.0-only",
    "Apache License": "Apache",
    "Apache License (== 2.0)": "Apache-2.0",
    "Apache License (>= 2)": "Apache",
    "Apache License (>= 2.0)": "Apache",
    "Apache License 2.0": "Apache-2.0",
    "Artistic-1.0": "Artistic-1.0-Perl",
    "Artistic-2.0": "Artistic-2.0",
    "BSD 2-clause License + file LICENSE": "BSD-2-Clause",
    "BSD_2_clause + file LICENSE": "BSD-2-Clause",
    "BSD_3_clause + file LICENSE": "BSD-3-Clause",
    "BSL-1.0": "BSL-1.0",
    "CC BY-NC-SA 4.0": "CC-BY-NC-SA-4.0",
    "CC0": "CC0-1.0",
    "CeCILL": "CECILL-2.0",
    "CeCILL-2": "CECILL-2.0",
    "EPL": "EPL",
    "GNU General Public License version 2": "GPL-2.0-only",
    "GPL": "GPL-2.0-or-later",
    "GPL (>= 2)": "GPL-2.0-or-later",
    "GPL (>= 2.0)": "GPL-2.0-or-later",
    "GPL (>= 3)": "GPL-3.0-or-later",
    "GPL (>= 3.0)": "GPL-3.0-or-later",
    "GPL (>=2)": "GPL-2.0-or-later",
    "GPL (>=3)": "GPL-3.0-or-later",
    "GPL-2": "GPL-2.0-only",
    "GPL-2 | GPL-3": "GPL-2.0-only OR GPL-3.0-only",
    "GPL-3": "GPL-3.0-only",
    "LGPL": "LGPL-2.0-or-later",
    "LGPL (>= 2)": "LGPL-2.0-or-later",
    "LGPL (>= 2.1)": "LGPL-2.1-or-later",
    "LGPL-2": "LGPL-2.0-only",
    "LGPL-2.1": "LGPL-2.1-only",
    "LGPL-3": "LGPL-3.0-only",
    "LGPL-3 | Apache License 2.0": "LGPL-3.0-only OR Apache-2.0",
    "Lucent Public License": "custom:LPL",
    "MIT + file LICENSE": "MIT",
    "Mozilla Public License 2.0": "MPL-2.0",
    "Unlimited": "LicenseRef-Unlimited",
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
        expect_needscompilation: bool = None,
        expect_systemrequirements: str = None,
        expect_title: str = None,
        extra_r_depends_cb = None,
        extra_r_makedepends = [],
        ignore_fortran_files: bool = False,
    ):
        self.expect_license = expect_license
        self.expect_needscompilation = expect_needscompilation
        self.expect_systemrequirements = expect_systemrequirements
        self.expect_title = expect_title
        self.extra_r_depends_cb = extra_r_depends_cb
        self.extra_r_makedepends = extra_r_makedepends
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
    expected_depends = set(desc.depends + desc.imports)
    cb = cfg.extra_r_depends_cb
    if cb is not None:
        expected_depends.update((_r_name_to_arch(dep) for dep in cb(cfg.tar)))
    errors = []
    for dep in pkg.depends:
        if dep.startswith("r-"):
            if dep not in expected_depends:
                errors.append(f"Unnecessary dependency: {dep}")
            else:
                implicit_r_dep = True
        elif dep == "r":
            explicit_r_dep = True

    not_missing = True
    for dep in expected_depends:
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
        if dep in pkg.depends or (dep.startswith("r-") and (dep not in desc.linkingto and dep not in cfg.extra_r_makedepends)):
            errors.append(f"Unnecessary make dependency: {dep}")

    for dep in desc.linkingto + cfg.extra_r_makedepends:
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
        if (dep not in cfg.default_r_pkgs) and (dep not in pkg.optdepends) and (dep not in pkg.depends):
            errors.append(f"Missing optional dependency: {dep}")

    if len(errors) > 0:
        raise CheckFailed('\n'.join(errors))

def has_fortran_files(tar: tarfile.TarFile, pkgname: str) -> bool:
    prefix = f"{pkgname}/src/"
    # accepted Fortran file suffixes are listed in `/etc/R/Makeconf`
    suffixes = (".f", ".f90", ".f95")
    return any(name.startswith(prefix) and name.endswith(suffixes) for name in tar.getnames())

def check_fortran(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    fortran_files = has_fortran_files(cfg.tar, pkg._pkgname)
    fortran_dep = "gcc-fortran" in pkg.makedepends

    if fortran_files and not fortran_dep:
        if not cfg.ignore_fortran_files:
            raise CheckFailed("Missing make dependency: gcc-fortran")
    elif not fortran_files and fortran_dep:
        raise CheckFailed("Unnecessary make dependency: gcc-fortran")
    elif cfg.ignore_fortran_files and not fortran_files:
        print("Warning: unnecessary config 'ignore_fortran_files'")

def _relevant_systemrequirements(value: str):
    """SystemRequirements without the C++ standard and GNU make, which every build has"""
    if value is None:
        return None
    parts = [part.strip() for part in value.split(",")]
    parts = [part for part in parts if part and not re.fullmatch(r"C\+\+\d\d|GNU make", part, re.I)]
    return ", ".join(parts) or None

def check_systemrequirements(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    sysrq = _relevant_systemrequirements(desc.systemrequirements)
    if _relevant_systemrequirements(cfg.expect_systemrequirements) != sysrq:
        if sysrq is None:
            # nothing new is required, so the build is not affected
            print("Warning: SystemRequirements have been removed, expect_systemrequirements is unnecessary")
        else:
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

def expected_pkgdesc(pkg: Pkgbuild, desc: Description) -> str:
    title = desc.title
    title_lower = title.lower()
    name_lower = pkg._pkgname.lower()
    # detect and remove package name from the title
    for sep in [",", ":", " -"]:
        prefix = f"{name_lower}{sep} "
        if title_lower.startswith(prefix):
            title = title[len(prefix):]
            break
    # detect and remove trailing dot from the title
    if len(title) > 1 and title[-1] == "." and title[-2] != ".":
        title = title[:-1]
    return title

def check_pkgdesc(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    title = expected_pkgdesc(pkg, desc)
    if cfg.expect_title is not None:
        if pkg.pkgdesc == title:
            print("Warning: unnecessary expect_title")
        elif cfg.expect_title != desc.title:
            raise CheckFailed(f"Title has changed: {desc.title}")
    elif pkg.pkgdesc != title:
        raise CheckFailed(f"Wrong pkgdesc, expected '{title}'")

def check_arch(pkg: Pkgbuild, desc: Description, cfg: CheckConfig):
    if cfg.expect_needscompilation is not None:
        if desc.needscompilation != cfg.expect_needscompilation:
            if desc.needscompilation != "" and pkg.arch == ["x86_64" if desc.needscompilation else "any"]:
                # arch already matches the new value
                print("Warning: unnecessary expect_needscompilation")
            else:
                raise CheckFailed(f"NeedsCompilation value has changed: {desc.needscompilation}")
    else:
        if desc.needscompilation == "":
            raise CheckFailed("No NeedsCompilation value")
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
    with tarfile.open(f"{pkgbuild._pkgname}_{newver}.tar.gz", "r:gz") as tar:
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

def r_fix_dependencies(pkg: Pkgbuild, desc: Description, cfg: CheckConfig, tar: tarfile.TarFile):
    """
    Automatically fix R package dependencies (r-*) in PKGBUILD file.
    Only handles r-* dependencies and gcc-fortran, preserving all other dependencies as-is.
    Returns a dict with the corrected dependencies.
    
    Args:
        pkg: Parsed PKGBUILD
        desc: Parsed DESCRIPTION from R package
        cfg: CheckConfig with additional configuration
        tar: Open tarfile for the R package
    
    Note: desc.depends and desc.imports are combined into a set, which
    automatically handles any duplicates from the DESCRIPTION file.
    """
    fixes = {}
    
    # Calculate expected R dependencies (set automatically deduplicates)
    expected_depends = set(desc.depends + desc.imports)
    cb = cfg.extra_r_depends_cb
    if cb is not None:
        expected_depends.update((_r_name_to_arch(dep) for dep in cb(tar)))
    
    # Fix depends array - only modify r-* packages
    new_depends = []
    implicit_r_dep = False
    
    for dep in pkg.depends:
        # Remove R dependencies that are in default R packages
        if dep in cfg.default_r_pkgs:
            continue
        
        if dep.startswith("r-"):
            # Only keep r-* dependencies that are expected
            if dep in expected_depends:
                new_depends.append(dep)
                implicit_r_dep = True
            # else: skip unnecessary r- dependencies
        elif dep == "r":
            # We'll handle r dependency separately based on check_depends logic
            pass
        else:
            # Keep all non-R dependencies as-is
            new_depends.append(dep)
    
    # Add missing R dependencies
    for dep in expected_depends:
        if (dep not in cfg.default_r_pkgs) and (dep not in new_depends):
            new_depends.append(dep)
            implicit_r_dep = True
    
    # Handle r dependency correctly (matching check_depends logic)
    # We need 'r' only if all dependencies are present and there's no implicit r dependency
    all_deps_present = all(
        (dep in cfg.default_r_pkgs or dep in new_depends) 
        for dep in expected_depends
    )
    if all_deps_present and not implicit_r_dep:
        new_depends.insert(0, 'r')
    
    # Sort dependencies for consistency
    def sort_key(dep):
        # Sort r first, then r-* packages, then others
        if dep == 'r':
            return (0, dep)
        elif dep.startswith('r-'):
            return (1, dep)
        else:
            return (2, dep)
    
    new_depends = sorted(new_depends, key=sort_key)
    fixes['depends'] = new_depends
    
    # Fix makedepends array - only modify r-* packages
    new_makedepends = []
    for dep in pkg.makedepends:
        if dep.startswith("r-"):
            # Remove r-* dependencies that are already in depends
            if dep in new_depends:
                continue
            # Remove r-* dependencies in default R packages
            if dep in cfg.default_r_pkgs:
                continue
            # Keep if it's in LinkingTo or extra makedepends
            if dep in desc.linkingto or dep in cfg.extra_r_makedepends:
                new_makedepends.append(dep)
        else:
            # Keep all non-R makedepends as-is (e.g., gcc-fortran)
            new_makedepends.append(dep)
    
    # Add missing R make dependencies
    for dep in desc.linkingto + cfg.extra_r_makedepends:
        if (dep not in cfg.default_r_pkgs) and (dep not in new_depends) and (dep not in new_makedepends):
            new_makedepends.append(dep)
    
    # gcc-fortran, as check_fortran expects it
    if has_fortran_files(tar, pkg._pkgname):
        if "gcc-fortran" not in new_makedepends and not cfg.ignore_fortran_files:
            new_makedepends.append("gcc-fortran")
    elif "gcc-fortran" in new_makedepends:
        new_makedepends.remove("gcc-fortran")
    
    new_makedepends = sorted(new_makedepends)
    fixes['makedepends'] = new_makedepends
    
    # Fix optdepends array - only modify r-* packages
    new_optdepends = []
    for dep in pkg.optdepends:
        if dep.startswith("r-"):
            # Remove r-* dependencies that are already in depends
            if dep in new_depends:
                continue
            # Remove r-* dependencies in default R packages  
            if dep in cfg.default_r_pkgs:
                continue
            # Keep if it's in Suggests
            if dep in desc.suggests:
                new_optdepends.append(dep)
        else:
            # Keep all non-R optdepends as-is
            new_optdepends.append(dep)
    
    # Add missing R optional dependencies
    for dep in desc.suggests:
        if (dep not in cfg.default_r_pkgs) and (dep not in new_optdepends) and (dep not in new_depends):
            new_optdepends.append(dep)
    
    new_optdepends = sorted(new_optdepends)
    fixes['optdepends'] = new_optdepends
    
    return fixes

def r_fix_metadata(pkg: Pkgbuild, desc: Description, cfg: CheckConfig) -> dict:
    """Return the pkgdesc and arch values expected by check_pkgdesc and check_arch, where they differ."""
    fixes = {}
    if cfg.expect_title is None:
        title = expected_pkgdesc(pkg, desc)
        if pkg.pkgdesc != title:
            fixes['pkgdesc'] = title
    if cfg.expect_needscompilation is None and desc.needscompilation != "":
        arch = "x86_64" if desc.needscompilation else "any"
        if pkg.arch != [arch]:
            fixes['arch'] = arch
    return fixes

# The PKGBUILD is edited as text rather than regenerated, so that everything the
# fixes do not touch stays as the maintainer wrote it.

def _array_end(text: str, i: int) -> int:
    """
    Return the index after the ')' that closes the bash array opened just before text[i].
    Quotes, backslash escapes and comments are skipped, so a ')' inside an optdepends
    description does not end the array.
    """
    quote = None
    while i < len(text):
        c = text[i]
        if quote == "'":
            if c == "'":
                quote = None
        elif c == "\\":
            i += 1
        elif quote == '"':
            if c == '"':
                quote = None
        elif c in "'\"":
            quote = c
        elif c == "#" and text[i - 1] in " \t\n(":
            # a comment ends at the end of the line
            i = text.find("\n", i)
            if i == -1:
                break
            continue
        elif c == ")":
            return i + 1
        i += 1
    raise Exception("Array is not closed")

def _find_definition(text: str, name: str):
    """
    Return the (start, end) of a top-level `name=...` definition in a PKGBUILD, or None.
    An array ends at its closing ')', a plain variable at the end of its line.
    """
    m = re.search(rf"^{name}=", text, re.M)
    if m is None:
        return None
    if text.startswith("(", m.end()):
        return m.start(), _array_end(text, m.end() + 1)
    end = text.find("\n", m.end())
    return m.start(), len(text) if end == -1 else end

def _quote_like(old: str, value: str) -> str:
    """Quote value for bash, in the style of the old value where possible."""
    if old.startswith("'") and "'" not in value:
        return f"'{value}'"
    if not any(c in value for c in "\"$`\\"):
        return f'"{value}"'
    return shlex.quote(value)

# where to create a missing array, in order of preference
_array_anchors = {
    "depends": ("license", "arch"),
    "makedepends": ("depends",),
    "optdepends": ("checkdepends", "makedepends", "depends"),
}

def r_rewrite_pkgbuild(text: str, fixes: dict) -> str:
    """
    Apply dependency and metadata fixes to the text of a PKGBUILD.
    Each array in fixes replaces the existing one, one entry per line, or is created
    after the arrays in _array_anchors when the PKGBUILD has none. pkgdesc and arch
    keep the quoting of the old value.
    """
    indent = "  "
    for name in ("depends", "makedepends", "optdepends"):
        if name not in fixes:
            continue
        block = f"{name}=(\n" + "".join(f"{indent}{shlex.quote(dep)}\n" for dep in fixes[name]) + ")"
        span = _find_definition(text, name)
        if span is not None:
            text = text[:span[0]] + block + text[span[1]:]
        elif fixes[name]:
            for anchor in _array_anchors[name]:
                span = _find_definition(text, anchor)
                if span is not None:
                    eol = text.find("\n", span[1])
                    if eol == -1:
                        eol = len(text)
                    text = text[:eol] + "\n" + block + text[eol:]
                    break
            else:
                raise Exception(f"Cannot place {name} in PKGBUILD")
    if "pkgdesc" in fixes:
        start, end = _find_definition(text, "pkgdesc")
        old = text[start + len("pkgdesc="):end]
        text = text[:start] + "pkgdesc=" + _quote_like(old, fixes["pkgdesc"]) + text[end:]
    if "arch" in fixes:
        start, end = _find_definition(text, "arch")
        q = "'" if "'" in text[start:end] else ""
        text = text[:start] + f"arch=({q}{fixes['arch']}{q})" + text[end:]
    return text

def r_apply_dependency_fixes(fixes: dict) -> bool:
    """
    Apply dependency fixes to PKGBUILD file.
    Returns whether the file changed.
    """
    with open("PKGBUILD") as f:
        text = f.read()
    new_text = r_rewrite_pkgbuild(text, fixes)
    if new_text != text:
        with open("PKGBUILD", "w") as f:
            f.write(new_text)
    return new_text != text

def r_auto_fix(newver: str, cfg: CheckConfig) -> bool:
    """
    Fix the dependencies, pkgdesc and arch from the DESCRIPTION.
    Returns whether the PKGBUILD changed.
    """
    pkgbuild = Pkgbuild()
    cfg.default_r_pkgs = get_default_r_pkgs()
    with tarfile.open(f"{pkgbuild._pkgname}_{newver}.tar.gz", "r:gz") as tar:
        description = Description(tar, pkgbuild._pkgname)
        cfg.tar = tar
        fixes = r_fix_dependencies(pkgbuild, description, cfg, tar)
        fixes.update(r_fix_metadata(pkgbuild, description, cfg))
    # leave arrays that are already right as they are written
    for name in ("depends", "makedepends", "optdepends"):
        if set(fixes[name]) == set(getattr(pkgbuild, name)):
            del fixes[name]
    return r_apply_dependency_fixes(fixes)

def _dep_name(dep: str) -> str:
    """Strip a version constraint, as in r-foo>=1.0"""
    return re.split("[<>=]", dep, maxsplit=1)[0]

def _in_repo(dep: str) -> bool:
    # lilac resolves repo_depends to directories of this repository
    return os.path.exists(os.path.join("..", dep, "lilac.yaml"))

def get_official_pkgs() -> set:
    """Get the names and provides of the packages in [core] and [extra]"""
    handle = pyalpm.Handle("/", str(PACMAN_DB_DIR))
    official = set()
    for repo in ("core", "extra"):
        for p in handle.register_syncdb(repo, 0).pkgcache:
            official.add(p.name)
            official.update(_dep_name(pr) for pr in p.provides)
    return official

# lilac.yaml is also edited as text: yaml.dump would remove comments and change the
# format of the rest of the file. Only top-level lists of plain entries are handled.

def r_check_packaged(pkg: Pkgbuild, official: set):
    """Fail if an R dependency is neither in this repository nor in [core] or [extra]."""
    missing = sorted({
        _dep_name(dep) for dep in pkg.depends + pkg.makedepends + pkg.checkdepends
        if dep.startswith("r-") and _dep_name(dep) not in official and not _in_repo(_dep_name(dep))
    })
    if missing:
        raise CheckFailed(f"Dependencies not packaged: {', '.join(missing)}")

# a list entry such as "- r-foo", "  - 'r-foo'" or "- r-foo  # comment"
_yaml_entry = re.compile(r"""^(\s*-\s+)(['"]?)([^\s'":#]+)\2\s*(#.*)?$""")

def _yaml_name(line: str):
    m = _yaml_entry.match(line)
    return m.group(3) if m else None

def _sync_yaml_list(lines: list, key: str, wanted: list, anchors: tuple) -> list:
    """
    Add the entries of wanted that a top-level list in lilac.yaml lacks, and remove
    its r-* entries that are not wanted, keeping other entries, comments and the
    order. Returns the entries it had before.
    """
    # find the key; a value on the same line (such as [a, b]) is not handled
    start = None
    for i, line in enumerate(lines):
        if line.startswith(f"{key}:"):
            start = i
    if start is not None and lines[start].split("#", 1)[0].rstrip() != f"{key}:":
        print(f"Warning: cannot update {key} in lilac.yaml")
        return wanted
    if start is None:
        # create the list after the first anchor key that exists
        if wanted:
            at = len(lines)
            for anchor in anchors:
                found = [i for i, line in enumerate(lines) if line.startswith(f"{anchor}:")]
                if found:
                    at = found[0] + 1
                    while at < len(lines) and lines[at][:1] in (" ", "\t", "-"):
                        at += 1
                    break
            lines[at:at] = [f"{key}:"] + [f"- {dep}" for dep in sorted(wanted)]
        return []

    # the list continues while lines are entries, indented lines or comments
    end = start + 1
    while end < len(lines) and lines[end][:1] in (" ", "\t", "-", "#"):
        end += 1
    block = lines[start + 1:end]
    present = [_yaml_name(line) for line in block if _yaml_name(line)]

    # new entries are written like the existing ones
    prefix, quote = "- ", ""
    for line in block:
        m = _yaml_entry.match(line)
        if m:
            prefix, quote = m.group(1) + m.group(2), m.group(2)
            break
        elif line.lstrip().startswith("-"):
            prefix = line[:line.index("-")] + "- "
            break

    # remove r-* entries that are not needed; other entries may be needed for other reasons
    block = [line for line in block if not (_yaml_name(line) or "").startswith("r-") or _yaml_name(line) in wanted]

    # add the missing ones: in sorted position if the list is sorted, else at its end
    in_order = present == sorted(present)
    for dep in sorted(set(wanted) - set(present)):
        entries = [i for i, line in enumerate(block) if line.lstrip().startswith("-")]
        at = entries[-1] + 1 if entries else 0
        if in_order:
            later = [i for i in entries if (_yaml_name(block[i]) or "") > dep]
            if later:
                at = later[0]
        block.insert(at, f"{prefix}{dep}{quote}")

    if any(line.lstrip().startswith("-") for line in block):
        lines[start + 1:end] = block
    else:
        # an empty list would load as None, so remove the key
        lines[start:end] = [line for line in block if line.startswith("#")]
    return present

def r_update_lilac_yaml(pkg: Pkgbuild, official: set = frozenset()):
    """
    Update repo_depends and repo_makedepends in lilac.yaml from the PKGBUILD.
    Dependencies that are packages of this repository, and not of [core] or [extra],
    are added; r-* entries that are no longer needed are removed. Other entries,
    comments and the order of entries are kept.
    Returns whether the file changed, and the packages that were not listed before.
    """
    if not os.path.exists("lilac.yaml"):
        return False, []
    def ours(dep):
        return dep not in official and _in_repo(dep)
    depends = [dep for dep in dict.fromkeys(map(_dep_name, pkg.depends)) if ours(dep)]
    # repo_makedepends takes what is needed only to build or check
    makedepends = [_dep_name(dep) for dep in pkg.makedepends + pkg.checkdepends]
    makedepends = [dep for dep in dict.fromkeys(makedepends) if ours(dep) and dep not in depends]

    with open("lilac.yaml") as f:
        text = f.read()
    lines = text.splitlines()
    before = set(_sync_yaml_list(lines, "repo_depends", depends, ("maintainers",)))
    before.update(_sync_yaml_list(lines, "repo_makedepends", makedepends, ("repo_depends", "maintainers")))
    new_text = "\n".join(lines) + "\n"
    if new_text != text:
        with open("lilac.yaml", "w") as f:
            f.write(new_text)
    return new_text != text, sorted(set(depends + makedepends) - before)

def _repo_entries(pkgbase: str, key: str) -> list:
    """The pkgbase names in a list of a package's lilac.yaml; an entry may be {pkgbase: pkgname}"""
    try:
        with open(os.path.join("..", pkgbase, "lilac.yaml")) as f:
            conf = yaml.safe_load(f) or {}
    except FileNotFoundError:
        return []
    return [next(iter(e)) if isinstance(e, dict) else e for e in conf.get(key) or []]

def r_chroot_packages(pkgbase: str) -> set:
    """
    The packages of this repository that lilac installs to build pkgbase, as its
    get_dependency_map does: repo_depends and repo_makedepends, and the repo_depends
    of each of those, recursively.
    """
    found = set()
    def add(dep):
        if dep not in found:
            found.add(dep)
            for d in _repo_entries(dep, "repo_depends"):
                add(d)
    for dep in _repo_entries(pkgbase, "repo_depends") + _repo_entries(pkgbase, "repo_makedepends"):
        add(dep)
    return found

def _restore_version(text: str, old: str) -> str:
    """Put back the version and checksum lines of old into text."""
    for name in ("_pkgver", "pkgrel", "md5sums", "sha256sums", "sha512sums", "b2sums"):
        span = _find_definition(text, name)
        old_span = _find_definition(old, name)
        if span is not None and old_span is not None:
            text = text[:span[0]] + old[old_span[0]:old_span[1]] + text[span[1]:]
    return text

def r_commit_lilac_yaml(newver: str):
    """
    Commit lilac.yaml with the fixed PKGBUILD. The PKGBUILD is committed with its
    old version and checksums, so that the next build updates it as usual.

    git_commit() from lilaclib is not used: its message needs _G.built_version,
    which is not set until after pre_build, and it commits the whole index.
    """
    pkgbase = os.path.basename(os.getcwd())
    with open("PKGBUILD") as f:
        text = f.read()
    old = run_cmd(["git", "show", "HEAD:./PKGBUILD"])
    try:
        with open("PKGBUILD", "w") as f:
            f.write(_restore_version(text, old))
        run_cmd(["git", "commit", "--no-gpg-sign", "-m", f"{pkgbase}: autofix for {newver}", "--", "PKGBUILD", "lilac.yaml"])
    finally:
        # the build under way needs the new version
        with open("PKGBUILD", "w") as f:
            f.write(text)

def r_pre_build(_G: SimpleNamespace, auto_fix: bool = True, **kwargs):
    """
    Update an R package to _G.newver and check the PKGBUILD against its DESCRIPTION.

    With auto_fix, the values that come from the DESCRIPTION are fixed, and the
    check runs again. Then repo_depends and repo_makedepends in lilac.yaml are
    updated to match the PKGBUILD. A changed lilac.yaml is committed here, because
    lilac does not commit it after the build. lilac chooses the packages for the
    build chroot before pre_build. So if a new package in lilac.yaml is not one of
    them, the build fails, and the next run uses it. Other errors of the check,
    such as a licence change, still fail the build. kwargs are passed to CheckConfig.
    """
    cfg = CheckConfig(**kwargs)
    newver, md5sum = _G.newver.rsplit("#", 1)
    cfg.md5sum = md5sum

    r_update_pkgver_and_pkgrel(newver)
    run_protected(["updpkgsums"])

    if not auto_fix:
        r_check_pkgbuild(newver, cfg)
        return

    # check, fix, check again; stop when a round of fixes changes nothing
    for _ in range(3):
        try:
            r_check_pkgbuild(newver, cfg)
            break
        except CheckFailed:
            if not r_auto_fix(newver, cfg):
                raise
    else:
        r_check_pkgbuild(newver, cfg)

    pkgbuild = Pkgbuild()
    official = get_official_pkgs()
    r_check_packaged(pkgbuild, official)
    # lilac has chosen the packages for the build chroot before pre_build
    in_chroot = r_chroot_packages(os.path.basename(os.getcwd()))
    changed, added = r_update_lilac_yaml(pkgbuild, official)
    if changed:
        r_commit_lilac_yaml(newver)
    missing = [dep for dep in added if dep not in in_chroot]
    if missing:
        raise CheckFailed(f"Committed new repo dependencies in lilac.yaml: {', '.join(missing)}. They are used from the next build.")
