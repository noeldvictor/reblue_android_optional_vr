"""Source-navigation and bounded cache tests; tiny temporary fixtures only."""
import contextlib
import io
import json
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

import callgraph as cg


class CallgraphTest(unittest.TestCase):
    def test_recursive_multiple_original_indirect_and_instruction_sites(self):
        source = """DEFINE_REX_FUNC(root) {
root(ctx, base); child(ctx, base); child(ctx, base);
__imp__root(ctx, base);
REX_CALL_INDIRECT_FUNC(ctr.u32);
LoadHook(ctx.r3);
}
DEFINE_REX_FUNC(child) {
}
"""
        graph = cg.parse_source(source, "unit.cpp", {"LoadHook"})
        self.assertEqual(graph["root"]["callees"], {"root": 2, "child": 2})
        self.assertEqual(graph["root"]["line"], 1)
        self.assertEqual([(s["kind"], s["target"], s["line"]) for s in graph["root"]["sites"]], [
            ("direct", "root", 2), ("direct", "child", 2), ("direct", "child", 2),
            ("original", "root", 3), ("indirect", "ctr.u32", 4), ("instruction-hook", "LoadHook", 5)])
        self.assertEqual(cg.subtree(graph, "root"), ["child"])
        self.assertEqual(cg.callers_of(graph, "root"), [("root", 2, "unit.cpp")])

    def test_comments_strings_raw_literals_and_declarations_are_not_calls(self):
        source = '''// DEFINE_REX_FUNC(fake) {
DEFINE_REX_FUNC(real) {
// fake(ctx, base);
/* fake(ctx, base); */
auto s = "fake(ctx, base);";
auto raw = R"tag(fake(ctx, base);
REX_CALL_INDIRECT_FUNC(ctr.u32);)tag";
actual(ctx, base);
}
'''
        graph = cg.parse_source(source, "unit.cpp")
        self.assertEqual(set(graph), {"real"})
        self.assertEqual(graph["real"]["callees"], {"actual": 1})
        self.assertEqual(graph["real"]["sites"][0]["line"], 8)

    def test_global_instruction_hook_prototype_is_not_a_call(self):
        source = """DEFINE_REX_FUNC(first) {
if (true) { value = {}; }
}
extern void SiteHook(PPCRegister &r3);
DEFINE_REX_FUNC(second) {
SiteHook(ctx.r3);
}
extern void SiteHook(PPCRegister &r3);
"""
        graph = cg.parse_source(source, "unit.cpp", {"SiteHook"})
        self.assertEqual(graph["first"]["sites"], [])
        self.assertEqual(graph["second"]["sites"], [{"kind": "instruction-hook", "target": "SiteHook", "line": 6}])

    def test_hook_declarations_and_local_token_pasting(self):
        source = '''// REX_HOOK_RAW(fake) {}
auto s = "REX_HOOK_RAW(fake)";
REX_HOOK(root, root_hook);
#define WRAP(name, operation) \\
  REX_HOOK_RAW(name) { body(operation); }
WRAP(child, One)
#define SAMPLER(Name) \\
  REX_HOOK_RAW(D3DDevice_SetSamplerState_##Name) {}
SAMPLER(Filter)
#if SOMETHING
REX_HOOK_RAW(conditional) {}
#endif
'''
        known = {"root", "child", "D3DDevice_SetSamplerState_Filter", "conditional", "fake"}
        declarations = cg.source_hook_declarations(source, "host.cpp", known)
        self.assertEqual({name for name, _ in declarations}, known - {"fake"})
        places = {name: location for name, location in declarations}
        self.assertEqual(places["child"], {"file": "host.cpp", "line": 6, "via": "WRAP"})
        self.assertEqual(places["D3DDevice_SetSamplerState_Filter"]["line"], 9)

    def test_frontier_stops_at_hook_declaration_but_does_not_call_it_complete(self):
        graph = cg.parse_source("""DEFINE_REX_FUNC(root) {
child(ctx, base); ext(ctx, base);
REX_CALL_INDIRECT_FUNC(ctr.u32);
SiteHook(ctx.r3);
}
DEFINE_REX_FUNC(child) {
hidden(ctx, base);
}
""", "unit.cpp", {"SiteHook"})
        declarations = {"child": [{"file": "host.cpp", "line": 3}]}
        rows = cg.frontier(graph, "root", declarations, 3)
        self.assertTrue(any(r[1:3] == ("host-hook?", "child") for r in rows))
        self.assertTrue(any(r[1:3] == ("indirect", "ctr.u32") for r in rows))
        self.assertTrue(any(r[1:3] == ("instruction-hook", "SiteHook") for r in rows))
        self.assertTrue(any(r[1:3] == ("external", "ext") for r in rows))
        self.assertFalse(any(r[2] == "hidden" for r in rows))
        output = io.StringIO()
        with contextlib.redirect_stdout(output):
            cg.tree(graph, "root", 16, budget=[1])
        self.assertEqual(len(output.getvalue().splitlines()), 1)

    def test_original_call_bypasses_hook_frontier(self):
        graph = cg.parse_source("""DEFINE_REX_FUNC(root) {
__imp__child(ctx, base);
}
DEFINE_REX_FUNC(child) {
hidden(ctx, base);
}
""", "unit.cpp")
        rows = cg.frontier(graph, "root", {"child": [{"file": "host.cpp", "line": 3}]}, 3)
        self.assertTrue(any(r[1:3] == ("guest-original", "child") for r in rows))
        self.assertTrue(any(r[1:3] == ("external", "hidden") for r in rows))

    def test_cache_opt_in_stamps_and_no_default_writes(self):
        with tempfile.TemporaryDirectory(prefix="reblue-callgraph-") as temporary:
            root = Path(temporary)
            gen = root / "generated"
            hooks = root / "hooks"
            gen.mkdir(); hooks.mkdir()
            cpp = gen / "unit.cpp"
            cpp.write_text("DEFINE_REX_FUNC(root) {\nSiteHook(ctx.r3);\n}\n", encoding="utf-8")
            hook = hooks / "unit.toml"
            hook.write_text('[[midasm_hook]]\nname = "SiteHook"\n', encoding="utf-8")
            cache = root / "cache.json"
            cache.write_text('{"old": "cache"}', encoding="utf-8")
            before = cache.read_bytes()
            with contextlib.redirect_stderr(io.StringIO()):
                graph = cg.load(gen=gen, cache=cache, hook_root=hooks)
                self.assertEqual(cache.read_bytes(), before)
                self.assertEqual(graph["root"]["sites"][0]["kind"], "instruction-hook")
                with patch.object(cg.shutil, "disk_usage", return_value=type("Usage", (), {"free": 30 << 30})()):
                    cg.load(write_cache=True, gen=gen, cache=cache, hook_root=hooks)
                with patch.object(cg, "build_index", side_effect=AssertionError("valid cache not reused")):
                    self.assertEqual(cg.load(gen=gen, cache=cache, hook_root=hooks), graph)
                saved = cache.read_bytes()
                hook.write_text('name = "DifferentHook"\n', encoding="utf-8")
                self.assertEqual(cg.load(gen=gen, cache=cache, hook_root=hooks)["root"]["sites"], [])
                self.assertEqual(cache.read_bytes(), saved)
                renamed = cpp.with_name("renamed.cpp")
                cpp.rename(renamed)
                self.assertEqual(cg.load(gen=gen, cache=cache, hook_root=hooks)["root"]["file"], "renamed.cpp")
            self.assertFalse(cache.with_name(cache.name + ".partial").exists())

    def test_cache_limits_foreign_partial_low_space_and_failed_replace(self):
        with tempfile.TemporaryDirectory(prefix="reblue-callgraph-") as temporary:
            cache = Path(temporary) / "cache.json"
            cache.write_bytes(b"old")
            self.assertFalse(cg.save_cache(cache, {"data": "x" * 100}, max_bytes=20))
            self.assertEqual(cache.read_bytes(), b"old")
            partial = cache.with_name(cache.name + ".partial")
            partial.write_bytes(b"foreign partial")
            self.assertFalse(cg.save_cache(cache, {}))
            self.assertEqual(partial.read_bytes(), b"foreign partial")
            partial.unlink()
            with patch.object(cg.shutil, "disk_usage", return_value=type("Usage", (), {"free": 0})()):
                self.assertFalse(cg.save_cache(cache, {}))
            with patch.object(cg.shutil, "disk_usage", return_value=type("Usage", (), {"free": 30 << 30})()):
                with patch.object(cg.os, "replace", side_effect=OSError("fixture")):
                    with self.assertRaises(OSError):
                        cg.save_cache(cache, {})
            self.assertEqual(cache.read_bytes(), b"old")
            self.assertFalse(partial.exists())

    def test_duplicate_definitions_and_input_budget_refuse(self):
        with self.assertRaisesRegex(ValueError, "duplicate"):
            cg.parse_source("DEFINE_REX_FUNC(a) {\n}\nDEFINE_REX_FUNC(a) {\n}", "unit.cpp")
        with tempfile.TemporaryDirectory(prefix="reblue-callgraph-") as temporary:
            root = Path(temporary)
            for name in ("one.cpp", "two.cpp"):
                (root / name).write_text("DEFINE_REX_FUNC(duplicate) {\n}\n", encoding="utf-8")
            with self.assertRaisesRegex(ValueError, "duplicate"):
                cg.build_index(root, hooks=())
            with patch.object(cg, "MAX_SOURCE_BYTES", 1):
                with self.assertRaisesRegex(ValueError, "input budget"):
                    cg.build_index(root, hooks=())

    def test_partial_race_never_deletes_another_writers_file(self):
        with tempfile.TemporaryDirectory(prefix="reblue-callgraph-") as temporary:
            cache = Path(temporary) / "cache.json"
            partial = cache.with_name(cache.name + ".partial")
            original_open = Path.open

            def competing_open(path, *args, **kwargs):
                if path == partial and args == ("xb",):
                    with original_open(path, "wb") as other:
                        other.write(b"other writer")
                return original_open(path, *args, **kwargs)

            with patch.object(cg.shutil, "disk_usage", return_value=type("Usage", (), {"free": 30 << 30})()):
                with patch.object(Path, "open", competing_open):
                    with self.assertRaises(FileExistsError):
                        cg.save_cache(cache, {})
            self.assertEqual(partial.read_bytes(), b"other writer")


if __name__ == "__main__":
    unittest.main()
