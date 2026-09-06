#!/usr/bin/env python3
"""Call graph over the recompiled guest, for planning a rewrite.

`generated/` is the whole XEX translated to C++ - 18,777 functions, every
address from config/functions.toml applied as a real name. That makes the call
graph extractable with a regex rather than a decompiler, and having it is what
makes rewriting a rendering path tractable: before replacing a function you need
to know who calls it, what it calls, and how much of the frame hangs off it.

  python tools/callgraph.py callers bdSceneNodeDrawSingle
  python tools/callgraph.py callees bdSceneNodeDrawSingle
  python tools/callgraph.py tree    bdSceneNodeCullTraverse --depth 3
  python tools/callgraph.py subtree bdSceneNodeDrawSingle    # everything reachable
  python tools/callgraph.py hot                              # static call-site counts
  python tools/callgraph.py frontier bdLoadModelDataCallback --depth 3
  python tools/callgraph.py sites bdSceneGraphNodeProcess

Queries rebuild stale indexes in memory by default. --cache explicitly permits
a bounded, atomic replacement of out/callgraph.json. No per-run copies or dumps.
"hot" counts static call sites, NOT runtime hotness. "frontier" stops at host
hook declarations and reports unresolved indirect calls and instruction hooks;
a declaration is neither proof of linked activation nor a guest-free body.

Reading the graph, not guessing at it, is the point. The renderer rewrite has to
replace whole paths - a function that batches instead of submitting per node
changes the contract for everything above it - and "what else touches this"
is not answerable by eye across 223 files.
"""

import argparse
import json
import os
import re
import sys
import time
import shutil
from pathlib import Path

GEN = "generated"
CACHE = os.path.join("out", "callgraph.json")
SCHEMA = 3
MAX_CACHE_BYTES = 8 << 20
MAX_SOURCE_BYTES = 256 << 20

# DEFINE_REX_FUNC(name) { ... } opens a function; the recompiler emits one per
# guest function, named where config/functions.toml names it and sub_ADDR
# otherwise.
DEF_RE = re.compile(r"^DEFINE_REX_FUNC\((\w+)\)")
# A call is `name(ctx, base);` at statement level. __imp__ prefixed calls are
# the host taking over a replaced function, and are counted as the same edge.
CALL_RE = re.compile(r"\b([A-Za-z_]\w*)\s*\(\s*ctx\s*,\s*base\s*\)")
INDIRECT_RE = re.compile(r"\bREX_CALL_INDIRECT_FUNC\s*\(([^)]+)\)")
HOOK_RE = re.compile(r"\bREX_HOOK(?:_RAW)?\s*\(\s*([\w#\s]+?)(?=[,)])")
# Mask comments/literals while preserving offsets and line numbers. This is a
# source-navigation index, not a C++ preprocessor or a semantic call graph.
NON_CODE_RE = re.compile(
    r'//[^\n]*|/\*[\s\S]*?\*/|R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\('
    r'[\s\S]*?\)(?P=delimiter)"|"(?:\\[\s\S]|[^"\\])*"|\'(?:\\.|[^\'\\])*\'')


def mask_non_code(source):
    return NON_CODE_RE.sub(lambda m: ''.join('\n' if c == '\n' else ' ' for c in m[0]), source)


def instruction_hooks(root="config/hooks"):
    # Only names from the actual hook manifests; no guess based on a suffix.
    names = set()
    for path in sorted(Path(root).glob("*.toml")):
        for line in path.read_text(encoding="utf-8").splitlines():
            match = re.match(r'\s*name\s*=\s*"(\w+)"', line)
            if match:
                names.add(match[1])
    return names


def parse_source(source, name, hooks=()):
    graph = {}
    current = None
    brace_depth = 0
    hook_re = re.compile(r"\b(" + "|".join(map(re.escape, sorted(hooks))) + r")\s*\(") if hooks else None
    for number, line in enumerate(mask_non_code(source).splitlines(), 1):
        match = DEF_RE.match(line)
        if match:
            if match[1] in graph:
                raise ValueError("duplicate generated function definition")
            current = graph[match[1]] = {"callees": {}, "file": name, "line": number, "sites": []}
            brace_depth = line.count("{") - line.count("}")
            if brace_depth == 0:
                current = None
            continue
        if current is None:
            continue
        for call in CALL_RE.finditer(line):
            raw = call[1]
            original = raw.startswith("__imp__")
            target = raw[7:] if original else raw
            current["callees"][target] = current["callees"].get(target, 0) + 1
            # Real recursive calls must remain edges, unlike the old index.
            current["sites"].append({"kind": "original" if original else "direct", "target": target, "line": number})
        for call in INDIRECT_RE.finditer(line):
            current["sites"].append({"kind": "indirect", "target": call[1].strip(), "line": number})
        if hook_re:
            for call in hook_re.finditer(line):
                current["sites"].append({"kind": "instruction-hook", "target": call[1], "line": number})
        brace_depth += line.count("{") - line.count("}")
        if brace_depth == 0:
            current = None  # following extern hook prototypes are not calls
    return graph


def build_index(gen=GEN, hooks=None):
    graph = {}
    files = sorted(Path(gen).glob("*.cpp"))
    if not files:
        sys.exit("no generated sources - run the reblue_codegen target first")
    if sum(p.stat().st_size for p in files) > MAX_SOURCE_BYTES:
        raise ValueError("generated source exceeds bounded input budget")
    hooks = instruction_hooks() if hooks is None else hooks
    for path in files:
        parsed = parse_source(path.read_text(encoding="utf-8"), path.name, hooks)
        if graph.keys() & parsed.keys():
            raise ValueError("duplicate generated function definition")
        graph.update(parsed)
    return graph


def source_stamp(gen, hook_root):
    paths = sorted(Path(gen).glob("*.cpp")) + sorted(Path(hook_root).glob("*.toml"))
    return [[str(p), p.stat().st_size, p.stat().st_mtime_ns] for p in paths]


def save_cache(path, payload, max_bytes=MAX_CACHE_BYTES):
    encoded = json.dumps(payload, separators=(",", ":")).encode("utf-8")
    if len(encoded) > max_bytes:
        return False
    path = Path(path)
    # One existing index plus one bounded temporary file. Never overwrite a
    # foreign partial file or follow a symlink; retain the prior cache on error.
    temporary = path.with_name(path.name + ".partial")
    if path.is_symlink() or temporary.exists() or temporary.is_symlink():
        return False
    if path.exists() and path.stat().st_size > max_bytes:
        return False
    for ancestor in (path.parent, *path.parents):
        if ancestor.exists() and (ancestor.is_symlink() or getattr(ancestor.lstat(), "st_file_attributes", 0) & 0x400):
            return False
    existing_parent = next((p for p in path.parents if p.exists()), None)
    if existing_parent is None or shutil.disk_usage(existing_parent).free < (20 << 30) + len(encoded):
        return False
    path.parent.mkdir(parents=True, exist_ok=True)
    created = False
    try:
        with temporary.open("xb") as output:
            created = True
            output.write(encoded)
        os.replace(temporary, path)
    finally:
        if created and temporary.exists():
            temporary.unlink()
    return True


def load(refresh=False, write_cache=False, gen=GEN, cache=CACHE, hook_root="config/hooks"):
    stamp = source_stamp(gen, hook_root)
    if not refresh and Path(cache).is_file() and Path(cache).stat().st_size <= MAX_CACHE_BYTES:
        try:
            payload = json.loads(Path(cache).read_text(encoding="utf-8"))
            if payload.get("schema") == SCHEMA and payload.get("stamp") == stamp:
                graph = payload["graph"]
                if isinstance(graph, dict) and all(
                    isinstance(fn, dict) and isinstance(fn.get("callees"), dict) and
                    isinstance(fn.get("sites"), list) and isinstance(fn.get("line"), int)
                    for fn in graph.values()):
                    return graph
        except (OSError, ValueError, KeyError, AttributeError):
            pass
    t0 = time.time()
    graph = build_index(gen, instruction_hooks(hook_root))
    stored = False
    if write_cache:
        try:
            stored = save_cache(cache, {"schema": SCHEMA, "stamp": stamp, "graph": graph})
        except OSError as error:
            print("cache not replaced: %s" % error, file=sys.stderr)
    print("indexed %d functions in %.1fs; %s"
          % (len(graph), time.time() - t0, "cache replaced" if stored else "memory only; cache unchanged"), file=sys.stderr)
    return graph


def source_hook_declarations(source, file, known):
    """Direct declarations and local one-level wrapper macros (including ##).

    Deliberately not a preprocessor: conditional declarations are candidates,
    and cross-header/nested macro expansion is outside this index's coverage.
    """
    clean = mask_non_code(source)
    macros = {}
    spans = []
    define_re = re.compile(r"^[ \t]*#[ \t]*define\s+(\w+)\(([^)]+)\)((?:[^\n]*\\\n)*[^\n]*)", re.M)
    for define in define_re.finditer(clean):
        spans.append((define.start(), define.end()))
        hook = HOOK_RE.search(define[3])
        if hook:
            macros[define[1]] = ([p.strip() for p in define[2].split(",")], hook[1])
    for start, end in reversed(spans):
        clean = clean[:start] + ''.join('\n' if c == '\n' else ' ' for c in clean[start:end]) + clean[end:]
    declarations = []

    def add(target, offset, via):
        if target in known:
            declarations.append((target, {"file": file, "line": clean.count('\n', 0, offset) + 1, "via": via}))

    for hook in HOOK_RE.finditer(clean):
        add(hook[1].strip(), hook.start(), "REX_HOOK")
    for macro, (parameters, pattern) in macros.items():
        for invocation in re.finditer(r"\b" + re.escape(macro) + r"\s*\(([^\n]*)", clean):
            arguments = invocation[1].split(",")
            arguments[-1] = arguments[-1].split(")", 1)[0]
            target = pattern
            for parameter, argument in zip(parameters, arguments):
                target = re.sub(r"\b" + re.escape(parameter) + r"\b", argument.strip(), target)
            add(re.sub(r"\s|##", "", target), invocation.start(), macro)
    return declarations


def host_hooks(graph, root="src"):
    paths = sorted(Path(root).rglob("*.cpp"))
    if sum(p.stat().st_size for p in paths) > MAX_SOURCE_BYTES:
        raise ValueError("host source exceeds bounded input budget")
    declarations = {}
    for path in paths:
        for target, location in source_hook_declarations(path.read_text(encoding="utf-8"), path.as_posix(), graph):
            declarations.setdefault(target, []).append(location)
    return declarations


def frontier(graph, root, declarations, depth=2):
    rows = []
    seen = set()
    pending = [(root, 0, False)]
    while pending:
        name, level, original = pending.pop()
        if (name, original) in seen:
            continue
        seen.add((name, original))
        fn = graph.get(name)
        if name in declarations and not original:
            for location in declarations[name]:
                rows.append((level, "host-hook?", name, location["file"], location["line"]))
            continue
        if fn is None:
            rows.append((level, "external", name, "", 0))
            continue
        rows.append((level, "guest-original" if original else "guest-body", name, fn["file"], fn["line"]))
        for site in fn["sites"]:
            if site["kind"] in ("indirect", "instruction-hook", "original"):
                rows.append((level + 1, site["kind"], site["target"], fn["file"], site["line"]))
        if level < depth:
            calls = {(site["target"], site["kind"] == "original") for site in fn["sites"]
                     if site["kind"] in ("direct", "original")}
            pending.extend((callee, level + 1, bypass) for callee, bypass in reversed(sorted(calls)))
    return rows


def callers_of(graph, target):
    out = []
    for name, fn in graph.items():
        if target in fn["callees"]:
            out.append((name, fn["callees"][target], fn["file"]))
    return sorted(out, key=lambda r: -r[1])


def tree(graph, root, depth, seen=None, indent=0, budget=None):
    if budget is None:
        budget = [80]
    if budget[0] <= 0:
        return
    if seen is None:
        seen = set()
    if root in seen or depth < 0:
        return
    budget[0] -= 1
    seen.add(root)
    fn = graph.get(root)
    if not fn:
        print("%s%s  (no body - host implemented or an import)"
              % ("  " * indent, root))
        return
    print("%s%s  [%s]" % ("  " * indent, root, fn["file"]))
    if depth == 0:
        return
    for callee, n in sorted(fn["callees"].items(), key=lambda kv: -kv[1]):
        if budget[0] <= 0:
            return
        suffix = "" if n == 1 else "  x%d" % n
        if callee in seen:
            budget[0] -= 1
            print("%s%s%s  (above)" % ("  " * (indent + 1), callee, suffix))
            continue
        tree(graph, callee, depth - 1, seen, indent + 1, budget)


def subtree(graph, root):
    seen, stack = set(), [root]
    while stack:
        cur = stack.pop()
        if cur in seen:
            continue
        seen.add(cur)
        fn = graph.get(cur)
        if fn:
            stack.extend(fn["callees"])
    seen.discard(root)
    return sorted(seen)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("mode",
                    choices=["callers", "callees", "tree", "subtree", "hot", "sites", "frontier"])
    ap.add_argument("function", nargs="?")
    ap.add_argument("--depth", type=int, choices=range(17), default=2)
    ap.add_argument("--limit", type=int, choices=range(1, 501), default=80, metavar="1..500")
    ap.add_argument("--cache", action="store_true", help="allow <=8 MiB cache replacement (default: no writes)")
    ap.add_argument("--refresh", action="store_true",
                    help="reindex even if the cache looks current")
    args = ap.parse_args()

    graph = load(args.refresh, args.cache)

    if args.mode == "hot":
        counts = {}
        for fn in graph.values():
            for callee, n in fn["callees"].items():
                counts[callee] = counts.get(callee, 0) + n
        print("%-44s %s" % ("function", "call sites"))
        for name, n in sorted(counts.items(), key=lambda kv: -kv[1])[:args.limit]:
            print("%-44s %d" % (name, n))
        return

    if not args.function:
        sys.exit("that mode needs a function name")

    if args.mode == "frontier":
        if args.function not in graph:
            sys.exit("no generated definition for " + args.function)
        rows = frontier(graph, args.function, host_hooks(graph), args.depth)
        print("Source frontier: host-hook? is a declaration, NOT proof of linked/native-only execution.")
        print("Stops at declarations; inspect their helpers/fallbacks. Indirect targets remain unresolved.")
        print("One-level local hook macros only; conditional/header/nested expansions need review.")
        for level, kind, name, file, line in rows[:args.limit]:
            print("%s%-17s %-42s %s:%d" % ("  " * level, kind, name, file, line))
        print("%d frontier rows, %d shown; depth %d (not a completion census)" % (len(rows), min(len(rows), args.limit), args.depth))
        return
    if args.mode == "sites":
        fn = graph.get(args.function)
        if fn is None:
            sys.exit("no generated definition for " + args.function)
        print("%s [%s:%d]" % (args.function, fn["file"], fn["line"]))
        for site in fn["sites"][:args.limit]:
            print("  %-17s %-42s %s:%d" % (site["kind"], site["target"], fn["file"], site["line"]))
        print("%d sites, %d shown; static source, not runtime frequency" % (len(fn["sites"]), min(len(fn["sites"]), args.limit)))
        return

    if args.mode == "callers":
        rows = callers_of(graph, args.function)
        if not rows:
            print("nothing calls %s directly.\n"
                  "Either it is an entry point, or it is reached indirectly "
                  "through a vtable or jump table - the recompiler emits those "
                  "as an address lookup, which this tool cannot see."
                  % args.function)
            return
        print("%-44s %-6s %s" % ("caller", "sites", "file"))
        for name, n, f in rows[:args.limit]:
            print("%-44s %-6d %s" % (name, n, f))
        print("\n%d caller(s). Each is a place the contract changes if %s is "
              "replaced." % (len(rows), args.function))
    elif args.mode == "callees":
        fn = graph.get(args.function)
        if not fn:
            sys.exit("%s has no recompiled body - host implemented, or a name "
                     "that is not in the graph" % args.function)
        print("%s  [%s]" % (args.function, fn["file"]))
        for callee, n in sorted(fn["callees"].items(), key=lambda kv: -kv[1])[:args.limit]:
            print("  %-42s x%d" % (callee, n))
    elif args.mode == "tree":
        tree(graph, args.function, args.depth, budget=[args.limit])
        print("Tree output limited to %d rows." % args.limit)
    elif args.mode == "subtree":
        names = subtree(graph, args.function)
        print("%d function(s) reachable from %s" % (len(names), args.function))
        for n in names[:args.limit]:
            print("  " + n)
        if len(names) > args.limit:
            print("  ... and %d more" % (len(names) - args.limit))


if __name__ == "__main__":
    main()
