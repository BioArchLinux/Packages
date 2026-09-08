import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import types
import unittest
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "lilac_r_utils", ROOT / "lilac-extensions/lilac_r_utils.py"
)
utils = importlib.util.module_from_spec(SPEC)
# The writer needs no live Lilac service or pacman database.
with patch.dict(sys.modules, {
    "lilac2.const": types.SimpleNamespace(PACMAN_DB_DIR="/unused"),
    "lilaclib": types.SimpleNamespace(edit_file=None, run_protected=None),
    "pyalpm": types.ModuleType("pyalpm"),
}):
    SPEC.loader.exec_module(utils)


class DependencyQuotingTests(unittest.TestCase):
    template = "depends=(\n  old\n)\nmakedepends=(\n  old\n)\noptdepends=(\n  old\n)\n"

    def rewrite(self, text, fixes):
        output = io.StringIO()
        with patch.object(utils, "edit_file", return_value=iter(text.splitlines())):
            with contextlib.redirect_stdout(output):
                utils.r_apply_dependency_fixes(fixes)
        return output.getvalue()

    def assert_roundtrip(self, text, fixes):
        subprocess.run(["bash", "-n"], input=text, text=True, check=True)
        for name, expected in fixes.items():
            command = text + f'\nfor value in "${{{name}[@]}}"; do printf "%s\\0" "$value"; done\n'
            result = subprocess.run(
                ["bash", "--noprofile", "--norc"], input=command,
                text=True, capture_output=True, check=True,
            )
            actual = result.stdout.split("\0")[:-1]
            self.assertEqual(actual, expected)

    def test_knitr_descriptions(self):
        fixes = {"depends": ["r-xfun"], "makedepends": [], "optdepends": [
            "pandoc: R Markdown v2 and reStructuredText support",
            "rst2pdf: rst2pdf() support",
        ]}
        self.assert_roundtrip(self.rewrite(self.template, fixes), fixes)

    def test_shell_metacharacters_in_all_arrays(self):
        values = ["r-cli", "libfoo>=1.0", "tool: owner's helper", 'tool: "quoted"',
                  "tool: $HOME", "tool: $(printf injected)", "tool: `printf injected`",
                  "tool: semi;colon", "tool: back\\slash", "tool: * [abc]"]
        fixes = {name: values for name in ("depends", "makedepends", "optdepends")}
        self.assert_roundtrip(self.rewrite(self.template, fixes), fixes)

    def test_empty_arrays_and_idempotence(self):
        fixes = {"depends": ["r-cli"], "makedepends": [], "optdepends": []}
        once = self.rewrite(self.template, fixes)
        self.assertEqual(once, self.rewrite(once, fixes))
        self.assertIn("  r-cli\n", once)
        self.assert_roundtrip(once, fixes)

    def test_real_knitr_recipe(self):
        recipe = (ROOT / "BioArchLinux/r-knitr/PKGBUILD").read_text()
        # Read dependency values with Bash, as the real updater does.
        fixes = {}
        for name in ("depends", "makedepends", "optdepends"):
            command = recipe + f'\nfor value in "${{{name}[@]}}"; do printf "%s\\0" "$value"; done\n'
            result = subprocess.run(["bash"], input=command, text=True,
                                    capture_output=True, check=True)
            fixes[name] = result.stdout.split("\0")[:-1]
        rewritten = self.rewrite(recipe, fixes)
        self.assert_roundtrip(rewritten, fixes)
        self.assertIn("build() {", rewritten)
        self.assertIn("package() {", rewritten)


if __name__ == "__main__":
    unittest.main()
