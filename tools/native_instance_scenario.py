"""Bounded, read-only post-event instance verification; never launches a game."""
import argparse
from pathlib import Path
import re

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


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--texture-tables", action="store_true")
    mode.add_argument("--texture-tables-normal", action="store_true")
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
    except Pending as error:
        print(f"Pending: {error}")
        return 2
    except (ValueError, OSError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: post-event native instances " + ", ".join(f"{k}={v}" for k, v in result.items()))
    if tables is not None:
        print("PASS: post-event native texture tables " + ", ".join(f"{k}={v}" for k, v in tables.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
