"""Runner behavior tests; no renderer, subprocess, files, or fixtures on disk."""
import contextlib
import io
import unittest
from unittest.mock import patch

import host_checks as checks


class HostChecksTest(unittest.TestCase):
    def test_default_covers_rigid_dependencies_without_duplicates(self):
        modules = checks.select_modules(None)
        self.assertEqual(len(modules), len(set(modules)))
        self.assertEqual(set(modules), {name for names in checks.AREAS.values() for name in names})
        self.assertIn("native_instance_scenario_test", modules)

    def test_focus_and_repeated_selection(self):
        self.assertEqual(checks.select_modules(["model", "model", "instance"]),
                         checks.AREAS["model"] + checks.AREAS["instance"])

    def test_broad_selection_contains_every_rigid_guard(self):
        self.assertTrue(set(checks.select_modules(None)) <= set(checks.select_modules(None, True)))

    def test_empty_selection_and_empty_module_fail(self):
        with contextlib.redirect_stderr(io.StringIO()):
            self.assertEqual(checks.run_checks(()), 2)
            loader = unittest.mock.Mock()
            loader.loadTestsFromName.return_value = unittest.TestSuite()
            self.assertEqual(checks.run_checks(("empty",), loader=loader), 2)

    def test_import_error_is_not_a_pass(self):
        self.assertEqual(checks.run_checks(("_reblue_nonexistent_test_module",), stream=io.StringIO()), 1)

    def exercise(self, fail, keep_going=False):
        visited = []

        class Cases(unittest.TestCase):
            def test_a(self):
                visited.append("a")
                self.assertFalse(fail)

            def test_b(self):
                visited.append("b")

        loader = unittest.mock.Mock()
        loader.loadTestsFromName.return_value = unittest.defaultTestLoader.loadTestsFromTestCase(Cases)
        status = checks.run_checks(("fixture",), loader=loader, stream=io.StringIO(), keep_going=keep_going)
        return status, visited

    def test_success_failure_and_fail_fast(self):
        self.assertEqual(self.exercise(False), (0, ["a", "b"]))
        self.assertEqual(self.exercise(True), (1, ["a"]))
        self.assertEqual(self.exercise(True, True), (1, ["a", "b"]))

    def test_list_does_not_load_or_execute_tests(self):
        output = io.StringIO()
        with patch.object(checks, "run_checks") as run, contextlib.redirect_stdout(output):
            self.assertEqual(checks.main(["--area", "model", "--list"]), 0)
            run.assert_not_called()
        self.assertEqual(output.getvalue().splitlines(), list(checks.AREAS["model"]))

    def test_ambiguous_and_unknown_selection_are_errors(self):
        for argv in (["--area", "model", "--all-boundaries"], ["--area", "typo"]):
            with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit) as error:
                checks.main(argv)
            self.assertEqual(error.exception.code, 2)


if __name__ == "__main__":
    unittest.main()
