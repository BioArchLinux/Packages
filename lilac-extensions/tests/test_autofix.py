import contextlib
import importlib.util
import io
import os
from pathlib import Path
import sys
import tempfile
import types
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "lilac_r_utils", ROOT / "lilac-extensions/lilac_r_utils.py"
)
utils = importlib.util.module_from_spec(SPEC)
with patch.dict(sys.modules, {
    "lilac2.const": types.SimpleNamespace(PACMAN_DB_DIR="/unused"),
    "lilaclib": types.SimpleNamespace(edit_file=None, run_cmd=None, run_protected=None),
    "pyalpm": types.ModuleType("pyalpm"),
}):
    SPEC.loader.exec_module(utils)


class FakeTar:
    def __init__(self, names):
        self.names = names

    def getnames(self):
        return self.names


def pkgbuild(**kw):
    values = dict(_pkgname="foo", pkgdesc="Foo", arch=["any"], depends=[],
                  makedepends=[], checkdepends=[], optdepends=[])
    values.update(kw)
    return types.SimpleNamespace(**values)


def description(**kw):
    values = dict(title="Foo", depends=[], imports=[], linkingto=[], suggests=[],
                  needscompilation=False, systemrequirements=None)
    values.update(kw)
    return types.SimpleNamespace(**values)


def config(**kw):
    cfg = utils.CheckConfig(**kw)
    cfg.default_r_pkgs = {"r-stats"}
    return cfg


class FortranTests(unittest.TestCase):
    def test_added_when_sources_have_fortran(self):
        tar = FakeTar(["foo/src/a.f90"])
        fixes = utils.r_fix_dependencies(pkgbuild(), description(), config(), tar)
        self.assertEqual(fixes["makedepends"], ["gcc-fortran"])

    def test_removed_when_no_fortran(self):
        tar = FakeTar(["foo/src/a.c"])
        pkg = pkgbuild(makedepends=["gcc-fortran", "git"])
        fixes = utils.r_fix_dependencies(pkg, description(), config(), tar)
        self.assertEqual(fixes["makedepends"], ["git"])

    def test_ignore_fortran_files(self):
        tar = FakeTar(["foo/src/a.f"])
        fixes = utils.r_fix_dependencies(pkgbuild(), description(), config(ignore_fortran_files=True), tar)
        self.assertEqual(fixes["makedepends"], [])


class RewriteTests(unittest.TestCase):
    def test_missing_arrays_are_created(self):
        text = "arch=(any)\ndepends=(\n  r\n)\ncheckdepends=(\n  r-testthat\n)\nsource=(x)\n"
        fixes = {"makedepends": ["r-rcpp"], "optdepends": ["r-knitr"]}
        self.assertEqual(utils.r_rewrite_pkgbuild(text, fixes),
                         "arch=(any)\ndepends=(\n  r\n)\nmakedepends=(\n  r-rcpp\n)\n"
                         "checkdepends=(\n  r-testthat\n)\noptdepends=(\n  r-knitr\n)\nsource=(x)\n")

    def test_empty_missing_array_is_not_created(self):
        text = "depends=(\n  r\n)\n"
        self.assertEqual(utils.r_rewrite_pkgbuild(text, {"optdepends": []}), text)

    def test_one_line_array(self):
        text = "makedepends=(gcc-fortran)\nsource=(\"x\")\nbuild() {\n  true\n}\n"
        fixes = {"makedepends": []}
        self.assertEqual(utils.r_rewrite_pkgbuild(text, fixes),
                         "makedepends=(\n)\nsource=(\"x\")\nbuild() {\n  true\n}\n")

    def test_array_with_parentheses_in_values(self):
        text = "optdepends=(\n  'pandoc: needed (sometimes)'  # comment )\n  \"b)\"\n)\nsource=(x)\n"
        fixes = {"optdepends": ["r-knitr"]}
        self.assertEqual(utils.r_rewrite_pkgbuild(text, fixes),
                         "optdepends=(\n  r-knitr\n)\nsource=(x)\n")

    def test_pkgdesc_and_arch_keep_quoting(self):
        text = "pkgdesc='Old'\narch=('any')\n"
        fixes = {"pkgdesc": "New title", "arch": "x86_64"}
        self.assertEqual(utils.r_rewrite_pkgbuild(text, fixes),
                         "pkgdesc='New title'\narch=('x86_64')\n")
        text = 'pkgdesc="Old"\narch=(any)\n'
        fixes = {"pkgdesc": "It's \"new\"", "arch": "x86_64"}
        self.assertEqual(utils.r_rewrite_pkgbuild(text, fixes),
                         "pkgdesc='It'\"'\"'s \"new\"'\narch=(x86_64)\n")

    def test_metadata_fixes(self):
        pkg = pkgbuild(pkgdesc="Old", arch=["any"])
        desc = description(title="foo: New Title.", needscompilation=True)
        self.assertEqual(utils.r_fix_metadata(pkg, desc, config()),
                         {"pkgdesc": "New Title", "arch": "x86_64"})
        cfg = config(expect_title="foo: New Title.", expect_needscompilation=False)
        self.assertEqual(utils.r_fix_metadata(pkg, desc, cfg), {})

    def test_restore_version(self):
        old = "_pkgver=1.0\npkgrel=3\nmd5sums=('aaa')\nb2sums=('bbb'\n        'ccc')\n"
        new = "_pkgver=2.0\npkgrel=1\ndepends=(\n  r-new\n)\nmd5sums=('ddd')\nb2sums=('eee')\n"
        self.assertEqual(utils._restore_version(new, old),
                         "_pkgver=1.0\npkgrel=3\ndepends=(\n  r-new\n)\nmd5sums=('aaa')\n"
                         "b2sums=('bbb'\n        'ccc')\n")


class WarningTests(unittest.TestCase):
    def test_harmless_keywords_warn(self):
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            utils.check_systemrequirements(pkgbuild(), description(), config(expect_systemrequirements="libfoo"))
            utils.check_pkgdesc(pkgbuild(), description(), config(expect_title="Foo"))
            cfg = config(ignore_fortran_files=True)
            cfg.tar = FakeTar([])
            utils.check_fortran(pkgbuild(), description(), cfg)
            utils.check_arch(pkgbuild(arch=["any"]), description(needscompilation=False),
                             config(expect_needscompilation=True))
        self.assertEqual(output.getvalue().count("Warning"), 4)

    def test_systemrequirements_ignore_cxx_and_make(self):
        utils.check_systemrequirements(pkgbuild(), description(systemrequirements="xml2, GNU make, C++17"),
                                       config(expect_systemrequirements="xml2, GNU make, C++11"))
        utils.check_systemrequirements(pkgbuild(), description(systemrequirements="c++14"), config())
        with self.assertRaises(utils.CheckFailed):
            utils.check_systemrequirements(pkgbuild(), description(systemrequirements="C++17, libfoo"),
                                           config(expect_systemrequirements="C++17"))

    def test_changed_keywords_fail(self):
        with self.assertRaises(utils.CheckFailed):
            utils.check_systemrequirements(pkgbuild(), description(systemrequirements="libbar"),
                                           config(expect_systemrequirements="libfoo"))
        with self.assertRaises(utils.CheckFailed):
            utils.check_pkgdesc(pkgbuild(pkgdesc="Other"), description(title="New"),
                                config(expect_title="Old"))
        with self.assertRaises(utils.CheckFailed):
            utils.check_arch(pkgbuild(arch=["x86_64"]), description(needscompilation=False),
                             config(expect_needscompilation=True))


class LilacYamlTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        for name in ("r-foo", "r-inrepo", "r-other", "r-rcppbio", "r-testbio"):
            os.makedirs(os.path.join(self.tmp.name, name))
            Path(self.tmp.name, name, "lilac.yaml").touch()
        cwd = os.getcwd()
        os.chdir(os.path.join(self.tmp.name, "r-foo"))
        self.addCleanup(os.chdir, cwd)

    def update(self, text, pkg):
        Path("lilac.yaml").write_text(text)
        changed, added = utils.r_update_lilac_yaml(pkg)
        return changed, added, Path("lilac.yaml").read_text()

    def test_keeps_comments_order_and_other_entries(self):
        text = ("build_prefix: extra-x86_64\nmaintainers:\n- github: someone\n"
                "repo_depends:\n  # needed at runtime\n  - samtools\n  - r-other\n  - r-gone\n"
                "update_on:\n- source: rpkgs\n")
        pkg = pkgbuild(depends=["r-inrepo", "r-other", "r-arch", "samtools"],
                       makedepends=["r-rcppbio"], checkdepends=["r-testbio", "r-other"])
        changed, added, out = self.update(text, pkg)
        self.assertTrue(changed)
        self.assertEqual(added, ["r-inrepo", "r-rcppbio", "r-testbio"])
        self.assertEqual(out, (
            "build_prefix: extra-x86_64\nmaintainers:\n- github: someone\n"
            "repo_depends:\n  # needed at runtime\n  - samtools\n  - r-other\n  - r-inrepo\n"
            "repo_makedepends:\n- r-rcppbio\n- r-testbio\n"
            "update_on:\n- source: rpkgs\n"))

    def test_sorted_insertion_and_quotes(self):
        text = "repo_depends:\n- 'r-foo'\n- 'r-other'\n"
        pkg = pkgbuild(depends=["r-foo", "r-inrepo", "r-other"])
        changed, added, out = self.update(text, pkg)
        self.assertEqual(out, "repo_depends:\n- 'r-foo'\n- 'r-inrepo'\n- 'r-other'\n")

    def test_indent_of_other_entries_and_moved_entries(self):
        text = "repo_depends:\n  - samtools\nrepo_makedepends:\n- r-inrepo\n"
        pkg = pkgbuild(depends=["r-inrepo", "samtools"])
        changed, added, out = self.update(text, pkg)
        self.assertEqual((changed, added), (True, []))
        self.assertEqual(out, "repo_depends:\n  - r-inrepo\n  - samtools\n")

    def test_other_packages_of_this_repository(self):
        text = "repo_depends:\n- r-foo\n- oldtool\n"
        pkg = pkgbuild(depends=["r-foo", "gtk2", "armadillo"])
        Path(self.tmp.name, "armadillo").mkdir()
        Path(self.tmp.name, "armadillo", "lilac.yaml").touch()
        Path(self.tmp.name, "gtk2").mkdir()
        Path(self.tmp.name, "gtk2", "lilac.yaml").touch()
        Path("lilac.yaml").write_text(text)
        changed, added = utils.r_update_lilac_yaml(pkg, official={"armadillo"})
        # packages in [extra] are not listed, and other entries are only added
        self.assertEqual(added, ["gtk2"])
        self.assertEqual(Path("lilac.yaml").read_text(), "repo_depends:\n- r-foo\n- oldtool\n- gtk2\n")

    def test_chroot_packages(self):
        Path("..", "r-inrepo", "lilac.yaml").write_text("repo_depends:\n- r-other\n- r-foo\n")
        Path("..", "r-rcppbio", "lilac.yaml").write_text("repo_depends:\n- r-testbio\n")
        Path("lilac.yaml").write_text("repo_depends:\n- r-inrepo\nrepo_makedepends:\n- r-rcppbio\n")
        self.assertEqual(utils.r_chroot_packages("r-foo"),
                         {"r-inrepo", "r-other", "r-foo", "r-rcppbio", "r-testbio"})

    def test_removes_empty_key_and_is_idempotent(self):
        text = "maintainers:\n- github: someone\nrepo_depends:\n- r-gone\nupdate_on:\n- alias: r\n"
        changed, added, out = self.update(text, pkgbuild(depends=["r-arch"]))
        self.assertEqual((changed, added), (True, []))
        self.assertEqual(out, "maintainers:\n- github: someone\nupdate_on:\n- alias: r\n")
        changed, added, again = self.update(out, pkgbuild(depends=["r-arch"]))
        self.assertEqual((changed, again), (False, out))


class CommitTests(unittest.TestCase):
    def test_commit_restores_version(self):
        with tempfile.TemporaryDirectory() as tmp:
            pkgdir = os.path.join(tmp, "r-foo")
            os.mkdir(pkgdir)
            cwd = os.getcwd()
            os.chdir(pkgdir)
            self.addCleanup(os.chdir, cwd)
            new = "_pkgver=2.0\npkgrel=1\ndepends=(\n  r-new\n)\nmd5sums=('new')\n"
            old = "_pkgver=1.0\npkgrel=2\ndepends=(\n  r-old\n)\nmd5sums=('old')\n"
            Path("PKGBUILD").write_text(new)
            committed = []

            def run_cmd(cmd):
                if cmd[:2] == ["git", "show"]:
                    return old
                committed.append((cmd, Path("PKGBUILD").read_text()))
                return ""

            with patch.object(utils, "run_cmd", run_cmd):
                utils.r_commit_lilac_yaml("2.0")
            cmd, text = committed[0]
            self.assertEqual(cmd[cmd.index("-m") + 1], "r-foo: autofix for 2.0")
            self.assertEqual(cmd[-3:], ["--", "PKGBUILD", "lilac.yaml"])
            self.assertEqual(text, "_pkgver=1.0\npkgrel=2\ndepends=(\n  r-new\n)\nmd5sums=('old')\n")
            self.assertEqual(Path("PKGBUILD").read_text(), new)


if __name__ == "__main__":
    unittest.main()
