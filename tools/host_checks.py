"""Capture-free Python checks for the native rigid-object development loop.

Does not build C++, launch the game, change profiles, or qualify rendered pixels.
Run from any directory; keep output in the terminal instead of creating run logs.
"""
import argparse
from pathlib import Path
import sys
import time
import unittest

# Also applies when this script is invoked without Python's -B option.
sys.dont_write_bytecode = True
TOOLS = Path(__file__).resolve().parent
AREAS = {
    "model": ("native_model_material_boundary_test",),
    "geometry": ("native_vertex_input_boundary_test", "native_mesh_storage_boundary_test",
                 "native_draw_bindings_boundary_test", "native_pipeline_boundary_test"),
    "material": (
        "native_texture_table_boundary_test", "native_material_texture_boundary_test",
        "native_primitive_policy_boundary_test", "native_lit_shading_boundary_test",
        "native_rigid_boundary_test",
    ),
    "instance": ("native_instance_boundary_test",),
    "scenario": ("native_instance_scenario_test",),
}


def select_modules(areas, all_boundaries=False):
    """Stable, deduplicated selection; default is the complete rigid-path group."""
    if all_boundaries:
        names = [path.stem for path in sorted(TOOLS.glob("native_*_boundary_test.py"))]
        names += list(AREAS["scenario"])
    else:
        names = [name for area in (areas or AREAS) for name in AREAS[area]]
    return tuple(dict.fromkeys(names))


def run_checks(modules, *, keep_going=False, stream=None, loader=None):
    stream = stream if stream is not None else sys.stderr
    loader = loader if loader is not None else unittest.TestLoader()
    if not modules:
        print("No check modules selected; refusing an empty pass.", file=stream)
        return 2
    suite = unittest.TestSuite()
    for name in modules:
        checks = loader.loadTestsFromName(name)
        if not checks.countTestCases():
            print(f"No tests found in {name}; refusing an empty pass.", file=stream)
            return 2
        suite.addTest(checks)
    started = time.perf_counter()
    result = unittest.TextTestRunner(stream=stream, verbosity=2,
                                     failfast=not keep_going).run(suite)
    print(f"Host checks: {result.testsRun} executed in {time.perf_counter() - started:.3f}s. "
          "Python source/scenario checks only; C++ fixtures and GPU qualification are separate.",
          file=stream)
    return 0 if result.wasSuccessful() else 1


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--area", action="append", choices=tuple(AREAS),
                        help="Focused group, repeatable; default: all rigid-path groups.")
    parser.add_argument("--all-boundaries", action="store_true",
                        help="All native Python boundary guards plus the scenario checker tests.")
    parser.add_argument("--list", action="store_true", help="List selection without importing tests.")
    parser.add_argument("--keep-going", action="store_true", help="Report all failures; default: fail fast.")
    args = parser.parse_args(argv)
    if args.area and args.all_boundaries:
        parser.error("--area and --all-boundaries are mutually exclusive")
    modules = select_modules(args.area, args.all_boundaries)
    if args.list:
        print("\n".join(modules))
        return 0 if modules else 2
    # Existing scenario tests import siblings. Do not require a particular cwd.
    if str(TOOLS) not in sys.path:
        sys.path.insert(0, str(TOOLS))
    return run_checks(modules, keep_going=args.keep_going)


if __name__ == "__main__":
    raise SystemExit(main())
