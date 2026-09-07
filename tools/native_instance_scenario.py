"""Bounded, read-only post-event instance verification; never launches a game."""
import argparse
from pathlib import Path
import re
import math

MAX_LOG_BYTES = 400 * 1024
READY = "mode FieldActive field-state 0 stage bg41_01 player 1 event 0 movie 0 loader-busy 0 icon-visible 0"
METRIC = re.compile(
    r"\[native-instances\] (\d+) created (\d+) retired (\d+) live / (\d+) bytes; "
    r"(\d+) poses published (\d+) reused; (\d+) producer imports (\d+) refused; "
    r"(\d+) consumer reads (\d+) unavailable; (\d+) checks wrong (\d+);")
TABLE_METRIC = re.compile(
    r"\[native-texture-tables\] (\d+) published (\d+) retired (\d+) indexed / (\d+) bytes; "
    r"(\d+) replacements (\d+) refused; (\d+) lookups (\d+) fallback; (\d+) checks wrong (\d+); "
    r"(\d+) image checks wrong (\d+); (\d+) native image reads (\d+) unavailable;")
VERTEX_METRIC = re.compile(
    r"\[native-vertex-input-use\] (\d+) pipeline binds, (\d+) decode blocks, (\d+) pulled records")
MOVEMENT_METRIC = re.compile(
    r"\[autoplay\] t ([\d.]+) stage (\S+) ready ([01]) walking ([01]) episode (\d+) "
    r"walk-s ([\d.]+) moved (\d+) distance ([\d.]+) position (\S+)")


class Pending(ValueError):
    pass


def verify(text):
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("instance diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[11]:
                raise ValueError("consumer pose differs from current source pose")
            if values[7]:
                raise ValueError("instance producer or memory budget refused an update")
            metrics.append((index, values))
    if metrics:
        index, latest = metrics[-1]
        if latest[4] and not latest[8] and latest[9] >= 10000 and any(
                i < index and READY in line for i, line in contexts):
            raise ValueError("published poses have no consumers after 10000 lookups in the ready field; inspect native-instance-miss")
    a, b = recent_field_samples(contexts, metrics)
    if b[0] == 0 or b[2] == 0 or b[4] == 0 or b[8] - a[8] < 32 or b[10] - a[10] < 32:
        raise Pending("native instance reads/checks did not advance sufficiently")
    return {"created": b[0], "live": b[2], "bytes": b[3],
            "reads_delta": b[8] - a[8], "checks_delta": b[10] - a[10],
            "imports_delta": b[6] - a[6], "unavailable": b[9]}


def recent_field_samples(contexts, metrics):
    if len(contexts) < 3:
        raise Pending("need opening event and two post-event field observations")
    if READY not in contexts[-1][1]:
        raise Pending("post-event field readiness not established")
    # Reports come from different threads. Match complete observation windows,
    # not a narrow polling instant after the last context and before the next.
    # Every accepted metric must FOLLOW its own context and precede any later
    # context; an empty/new final window may wait without discarding prior proof.
    completed = []
    for number, context in enumerate(contexts):
        end = contexts[number + 1][0] if number + 1 < len(contexts) else float("inf")
        samples = [values for i, values in metrics if context[0] < i < end]
        if samples:
            completed.append((number, context, samples[-1]))
    if len(completed) < 2:
        raise Pending("need fresh samples after both field observations")
    earlier, later = completed[-2:]
    previous, current = earlier[1], later[1]
    if (later[0] != earlier[0] + 1 or later[0] < len(contexts) - 2 or
            not all(READY in row[1] for row in (previous, current)) or not any(
                i < previous[0] and "stage bg41_01 player 1 event 1 movie 0" in line
                for i, line in contexts)):
        raise Pending("need consecutive, recent post-event field observations")
    return earlier[2], later[2]


def verify_texture_tables(text, comparison=True):
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("texture-table diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = TABLE_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[5] or values[9] or values[11]:
                raise ValueError("native texture-table publication refused or source/image comparison differs")
            if not comparison and (values[7] or values[8] or values[10]):
                raise ValueError("normal texture-table path executed original fallback/comparison")
            metrics.append((index, values))
    if metrics:
        index, latest = metrics[-1]
        if latest[0] and not latest[6] and latest[7] >= 10000 and any(
                i < index and READY in line for i, line in contexts):
            raise ValueError("published texture tables have no field consumers; inspect the active table producer")
    a, b = recent_field_samples(contexts, metrics)
    required = (6, 8, 10) if comparison else (6,)
    if not b[0] or not b[2] or any(b[i] - a[i] < 32 for i in required):
        raise Pending("native table lookup and non-null image checks must advance in the ready field")
    return {"published": b[0], "indexed": b[2], "bytes": b[3],
            "lookups_delta": b[6] - a[6], "checks_delta": b[8] - a[8],
            "image_checks_delta": b[10] - a[10], "fallback": b[7],
            "native_image_reads_delta": b[12] - a[12]}


def verify_vertex_inputs(text, require_pulling=False):
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("vertex-input diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = VERTEX_METRIC.search(line)
        if match:
            metrics.append((index, tuple(map(int, match.groups()))))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] - a[0] < 32 or b[1] - a[1] < 32 or b[2] < a[2]:
        raise Pending("native pipeline and shader-input consumers must advance in the ready field")
    if require_pulling and b[2] - a[2] < 32:
        raise Pending("native vertex pulling must advance in the ready field")
    return {"pipeline_binds_delta": b[0] - a[0], "decode_blocks_delta": b[1] - a[1],
            "pulled_records_delta": b[2] - a[2]}


def verify_movement(text):
    """Require observed displacement during one fresh, uninterrupted field walk."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("movement diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = MOVEMENT_METRIC.search(line)
        if match:
            t, stage, ready, walking, episode, duration, moved, distance, position = match.groups()
            xyz = tuple(map(float, position.split(",")))
            values = (float(t), stage, int(ready), int(walking), int(episode),
                      float(duration), int(moved), float(distance))
            if len(xyz) != 3 or not all(math.isfinite(v) for v in xyz + (values[0], values[5], values[7])):
                raise ValueError("invalid movement observation")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if (a[1] != "bg41_01" or b[1] != a[1] or not all((a[2], a[3], b[2], b[3])) or
            not a[4] or b[4] != a[4] or b[0] <= a[0] or b[5] - a[5] < 1 or
            b[6] - a[6] < 4 or b[7] - a[7] < 0.1):
        raise Pending("need fresh displacement during the same ready-field walking episode")
    # A later loss of readiness must invalidate otherwise complete old windows.
    if not metrics[-1][1][2] or not metrics[-1][1][3] or metrics[-1][1][4] != b[4]:
        raise Pending("walking was interrupted after the qualifying windows")
    return {"episode": b[4], "samples_delta": b[6] - a[6],
            "distance_delta": round(b[7] - a[7], 6), "walk_seconds": b[5]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--texture-tables", action="store_true")
    mode.add_argument("--texture-tables-normal", action="store_true")
    parser.add_argument("--vertex-inputs", action="store_true")
    parser.add_argument("--vertex-pulling", action="store_true")
    parser.add_argument("--movement", action="store_true")
    args = parser.parse_args()
    try:
        with args.log.open("rb") as source:
            data = source.read(MAX_LOG_BYTES + 1)
        if len(data) > MAX_LOG_BYTES:
            raise ValueError("instance diagnostic exceeds 400 KiB")
        text = data.decode("utf-8")
        result = verify(text)
        tables = verify_texture_tables(text, comparison=not args.texture_tables_normal) if (
            args.texture_tables or args.texture_tables_normal) else None
        vertex_inputs = verify_vertex_inputs(text, args.vertex_pulling) if (
            args.vertex_inputs or args.vertex_pulling) else None
        movement = verify_movement(text) if args.movement else None
    except Pending as error:
        print(f"Pending: {error}")
        return 2
    except (ValueError, OSError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: post-event native instances " + ", ".join(f"{k}={v}" for k, v in result.items()))
    if tables is not None:
        print("PASS: post-event native texture tables " + ", ".join(f"{k}={v}" for k, v in tables.items()))
    if vertex_inputs is not None:
        print("PASS: post-event native vertex inputs " + ", ".join(f"{k}={v}" for k, v in vertex_inputs.items()))
    if movement is not None:
        print("PASS: post-event observed player movement " + ", ".join(f"{k}={v}" for k, v in movement.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
