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
        raise Pending("need fresh instance samples after both field observations")
    earlier, later = completed[-2:]
    previous, current = earlier[1], later[1]
    if (later[0] != earlier[0] + 1 or later[0] < len(contexts) - 2 or
            not all(READY in row[1] for row in (previous, current)) or not any(
                i < previous[0] and "stage bg41_01 player 1 event 1 movie 0" in line
                for i, line in contexts)):
        raise Pending("need consecutive, recent post-event field observations")
    a, b = earlier[2], later[2]
    if b[0] == 0 or b[2] == 0 or b[4] == 0 or b[8] - a[8] < 32 or b[10] - a[10] < 32:
        raise Pending("native instance reads/checks did not advance sufficiently")
    return {"created": b[0], "live": b[2], "bytes": b[3],
            "reads_delta": b[8] - a[8], "checks_delta": b[10] - a[10],
            "imports_delta": b[6] - a[6], "unavailable": b[9]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    args = parser.parse_args()
    try:
        with args.log.open("rb") as source:
            data = source.read(MAX_LOG_BYTES + 1)
        if len(data) > MAX_LOG_BYTES:
            raise ValueError("instance diagnostic exceeds 400 KiB")
        result = verify(data.decode("utf-8"))
    except Pending as error:
        print(f"Pending: {error}")
        return 2
    except (ValueError, OSError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: post-event native instances " + ", ".join(f"{k}={v}" for k, v in result.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
