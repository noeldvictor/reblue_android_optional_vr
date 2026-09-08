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
CANONICAL_METRIC = re.compile(
    r"\[native-mesh-canonical\] (\d+) meshes, (\d+) draws, (\d+) source-free disk loads;")
SHADOW_POLICY_METRIC = re.compile(
    r"\[native-model-shadow\] (\d+) load-owned policies \((\d+) disabled\), (\d+) unknown; "
    r"(\d+) draw lookups hit / (\d+) unavailable;")
SHADOW_RECEIVER_METRIC = re.compile(
    r"\[native-shadow\] receiver inputs checked (\d+) wrong (\d+) receiving (\d+); "
    r"replays composed (\d+) changed (\d+)")
MATERIAL_TEXTURE_METRIC = re.compile(
    r"\[native-material-textures\] (\d+) object publications (\d+) with overrides; "
    r"(\d+) unsupported (\d+) refused; (\d+) meshes prepared, peak (\d+) bytes; "
    r"(\d+) reads (\d+) unavailable; (\d+) checks wrong (\d+); "
    r"(\d+) draws (\d+) image slots (\d+) UV blocks;")
MOVEMENT_METRIC = re.compile(
    r"\[autoplay\] t ([\d.]+) stage (\S+) ready ([01]) walking ([01]) episode (\d+) "
    r"walk-s ([\d.]+) moved (\d+) distance ([\d.]+) position (\S+)")
PRIMITIVE_POLICY_METRIC = re.compile(
    r"\[native-primitive-policy\] (\d+) plans (\d+) known (\d+) unknown; "
    r"(\d+) direct (\d+) deferred (\d+) suppressed candidates; "
    r"(\d+) reads (\d+) unavailable; (\d+) checks wrong (\d+); "
    r"(\d+) draws (\d+) cull changes (\d+) compound refreshes;")
LIT_SHADING_METRIC = re.compile(r"\[native-lit-shading\] (\d+) normal-lit queued draws;")
DRAW_BINDING_METRIC = re.compile(r"\[draw-bindings\] (\d+) emitted draws (\d+) descriptor binds (\d+) layout binds;")
MODEL_NODE_METRIC = re.compile(r"\[native-model-nodes\] (\d+) owned bounds/primitive associations (\d+) unavailable; (\d+) bounds checks wrong (\d+);")
OBJECT_INPUT_METRIC = re.compile(r"\[native-object-inputs\] (\d+) publications (\d+) owned colour reads (\d+) unavailable; (\d+) checks wrong (\d+); (\d+) owned primitive packets;")
SELECTED_LIGHT_METRIC = re.compile(r"\[native-selected-lights\] (\d+) publications (\d+) changed slots (\d+) compatibility; (\d+) checks wrong (\d+); (\d+) object snapshots (\d+) unavailable; (\d+) draw checks wrong (\d+);")


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


def observe_occlusion(text):
    """Fresh culling context for a separate image; NOT reload/pixel acceptance."""
    if len(text.encode("utf-8")) > 2 * MAX_LOG_BYTES:
        raise ValueError("occlusion observation exceeds 800 KiB")
    if re.search(r"\[(?:error|critical)\]|\[native-[^\]]*-mismatch\]|\[shutdown\]", text):
        raise ValueError("runtime failure or shutdown before occlusion observation")
    current_depth = "[native-depth-vis]" in text
    pattern = re.compile(r"\[native-depth-vis\] frame (\d+) snapshots (\d+) generated (\d+) draw-recorded (\d+) "
                         r"fence-collected (\d+) visible-instances (\d+) culled-instances (\d+); buffers (\d+) owners (\d+);"
                         if current_depth else r"\[native-occ\] frame (\d+) requested (\d+) queried (\d+) "
                         r"fence-collected (\d+) zero (\d+) native-skipped (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = pattern.search(line)
        if current_depth and "[native-depth-vis]" in line and not match:
            raise ValueError("malformed current-depth visibility evidence")
        if match:
            values = tuple(map(int, match.groups()))
            if current_depth:
                if (not values[4] <= values[3] <= values[2] or
                        not values[4] <= values[5]+values[6] <= values[4]*256 or
                        values[7] > 64*1024*1024 or values[8] > 16):
                    raise ValueError("inconsistent or over-budget current-depth GPU counters")
            elif not (values[4] <= values[3] <= values[2] <= values[1]) or values[5] > values[1]:
                raise ValueError("inconsistent native query counters")
            metrics.append((index, values))
        # A context from before teardown cannot authorize a title/loading image.
        if "[native-rigid-reload] title requested" in line or "[native-rigid-lifecycle] source-retired" in line:
            contexts.append((index, "lifecycle transition"))
    a, b = recent_field_samples(contexts, metrics)
    monotonic_fields = 7 if current_depth else len(a)
    if metrics[-1][0] < contexts[-1][0] or any(y < x for x, y in zip(a[:monotonic_fields], b[:monotonic_fields])):
        raise Pending("need current, monotonic field occlusion samples")
    if current_depth:
        if b[0] <= a[0] or any(b[i] <= a[i] for i in (1,2,3,4,5,6)):
            raise Pending("current-depth snapshots, real draws, collected visible and culled instances must advance")
        return dict(mode="current-depth", frame=b[0], snapshots_delta=b[1]-a[1], generated_delta=b[2]-a[2],
                    draw_recorded_delta=b[3]-a[3], collected_delta=b[4]-a[4], visible_delta=b[5]-a[5], skipped_delta=b[6]-a[6])
    if b[0] <= a[0] or any(b[i] <= a[i] for i in (1, 2, 3, 5)):
        raise Pending("native queries, collection and skips must advance in the ready field")
    return dict(frame=b[0], requests_delta=b[1]-a[1], queries_delta=b[2]-a[2],
                collected_delta=b[3]-a[3], skipped_delta=b[5]-a[5])


def verify_frame_probe(text, request_text, image_size, image_extent):
    """Validate provenance only; the actual image still requires visual review."""
    if len(text.encode("utf-8")) > 2 * MAX_LOG_BYTES:
        raise ValueError("frame probe log exceeds 800 KiB")
    request = re.fullmatch(r"[ \r\n]*([0-9]+)[ \r\n]+([0-9]+)[ \r\n]+([0-9]+)[ \r\n]*", request_text)
    if len(request_text.encode("utf-8")) > 64 or not request:
        raise ValueError("malformed frame probe request")
    identity, first, last = map(int, request.groups())
    if not (0 < identity < 2**64 and 0 < first <= last < 2**32 and last-first <= 120):
        raise ValueError("frame probe request exceeds identity/frame limits")
    fields = (r"request (\d+) frame (\d+) slot (\d+) input ([0-9A-F]+) output ([0-9A-F]+) "
              r"descriptor (\d+) size (\d+)x(\d+)")
    recorded = re.compile(r"\[native-frame-probe\] recorded " + fields + r"; awaiting submission fence$")
    saved = re.compile(r"\[native-frame-probe\] saved " + fields + r" bytes (\d+); post-gamma, GPU fence complete$")
    rows = text.splitlines()
    records, receipts = [], []
    for index, line in enumerate(rows):
        if "[native-frame-probe]" not in line:
            continue
        record, receipt = recorded.search(line), saved.search(line)
        if record:
            records.append((index, record.groups()))
        elif receipt:
            receipts.append((index, receipt.groups()))
        else:
            raise ValueError("malformed or failed frame probe receipt")
    if len(records) > 1 or len(receipts) > 1:
        raise ValueError("frame probe must record and publish exactly once")
    if not records or not receipts:
        raise Pending("waiting for recorded frame and real-fence JPEG receipt")
    (record_index, record), (saved_index, receipt) = records[0], receipts[0]
    if record_index >= saved_index or record != receipt[:8]:
        raise ValueError("frame probe recorded/saved identities differ or arrive out of order")
    req, frame, slot, source, target, descriptor, width, height = (
        int(value, 16 if i in (3, 4) else 10) for i, value in enumerate(record))
    if (req != identity or not first <= frame <= last or slot >= 2 or
            not 0 < source < 2**64 or not 0 < target < 2**64 or source == target or
            descriptor >= 2**32-1 or not 0 < width <= 2048 or not 0 < height <= 1200):
        raise ValueError("frame probe identity, interval or native image contract is invalid")
    if image_extent != (width, height) or not 0 < image_size <= 110*1024 or image_size != int(receipt[8]):
        raise ValueError("frame probe JPEG dimensions/byte count do not match the fence receipt")
    observation = observe_occlusion("\n".join(rows[:record_index]))
    if observation.get("mode") != "current-depth" or observation["frame"] != first:
        raise ValueError("frame probe does not follow its fresh current-depth observation")
    for line in rows[record_index:saved_index+1]:
        if (("[native-material-context]" in line and READY not in line) or
                "[native-rigid-reload] title requested" in line or
                "[native-rigid-lifecycle] source-retired" in line or
                re.search(r"\[(?:error|critical)\]|\[shutdown\]", line)):
            raise ValueError("scene/lifetime changed before frame-probe collection")
    return dict(request=req, frame=frame, slot=slot, input=source, output=target,
                descriptor=descriptor, width=width, height=height, bytes=image_size)


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


def verify_canonical_geometry(text):
    """Observe live converted draws without implying source-free loading occurred."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("canonical-geometry diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = CANONICAL_METRIC.search(line)
        if match:
            metrics.append((index, tuple(map(int, match.groups()))))
    a, b = recent_field_samples(contexts, metrics)
    if not a[0] or not b[0] or b[1] - a[1] < 32 or any(b[i] < a[i] for i in (0, 2)):
        raise Pending("canonical mesh draws must advance in the ready field")
    return {"meshes": b[0], "draws_delta": b[1] - a[1],
            "source_free_loads_delta": b[2] - a[2]}


def verify_shadow_policies(text):
    """Match owned policy use AND receiver comparisons in the same field windows."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("shadow-policy diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    policy = None
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
            policy = None
        match = SHADOW_POLICY_METRIC.search(line)
        if match:
            policy = tuple(map(int, match.groups()))
        match = SHADOW_RECEIVER_METRIC.search(line)
        if match:
            receiver = tuple(map(int, match.groups()))
            if receiver[1]:
                raise ValueError("native shadow receiver differs from source")
            if policy is not None:
                metrics.append((index, policy + receiver))
    a, b = recent_field_samples(contexts, metrics)
    if not a[0] or not b[0] or any(b[i] < a[i] for i in range(len(b))) or any(
            b[i] - a[i] < 32 for i in (3, 5, 7, 8)):
        raise Pending("owned shadow lookups, matching receiver checks and receiving replays must advance")
    return {"policies": b[0], "disabled_policies": b[1], "unknown": b[2],
            "lookups_delta": b[3] - a[3], "unavailable": b[4],
            "checks_delta": b[5] - a[5], "receiving_delta": b[7] - a[7],
            "replays_delta": b[8] - a[8]}


def verify_material_textures(text):
    """Require fresh native publication, matching capture and image/UV consumption."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("material-texture diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        if "[native-material-texture-mismatch]" in line:
            raise ValueError("native material image/UV differs from source")
        match = MATERIAL_TEXTURE_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[3] or values[9] or values[5] > 4 * 1024 * 1024:
                raise ValueError("material-texture publication refused, exceeded budget or differs from source")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if not b[4] or any(b[i] < a[i] for i in range(len(b))) or any(
            b[i] - a[i] < 32 for i in (0, 6, 8, 10, 11, 12)):
        raise Pending("native object publications, matching checks and image/UV draws must advance")
    return {"publications_delta": b[0] - a[0], "override_publications_delta": b[1] - a[1],
            "unsupported": b[2], "peak_bytes": b[5], "reads_delta": b[6] - a[6],
            "unavailable": b[7], "checks_delta": b[8] - a[8], "draws_delta": b[10] - a[10],
            "image_slots_delta": b[11] - a[11], "uv_blocks_delta": b[12] - a[12]}


def verify_primitive_policies(text):
    """Observe owned policy checks/consumers, not unexercised routing families."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("primitive-policy diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        if "[native-primitive-policy-mismatch]" in line:
            raise ValueError("native primitive cull/participation differs from source")
        match = PRIMITIVE_POLICY_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[9]:
                raise ValueError("native primitive cull/participation comparison differs")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if any(b[i] < a[i] for i in range(len(b))) or any(
            b[i] - a[i] < 32 for i in (0, 1, 6, 8, 10)):
        raise Pending("native plans, known routing, matching checks and policy draws must advance")
    return {"plans_delta": b[0] - a[0], "known_delta": b[1] - a[1],
            "unknown_delta": b[2] - a[2], "direct_candidates_delta": b[3] - a[3],
            "deferred_candidates_delta": b[4] - a[4], "suppressed_candidates_delta": b[5] - a[5],
            "reads_delta": b[6] - a[6], "unavailable": b[7], "checks_delta": b[8] - a[8],
            "draws_delta": b[10] - a[10], "cull_changes_delta": b[11] - a[11],
            "compound_refreshes_delta": b[12] - a[12]}


def verify_lit_shading(text):
    """Live queued use, not a numerical pixel comparison or a complete native draw."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("lit-shading diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = LIT_SHADING_METRIC.search(line)
        if match:
            metrics.append((index, tuple(map(int, match.groups()))))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] - a[0] < 32:
        raise Pending("normal lit shading must have fresh queued field draws")
    return {"queued_draws_delta": b[0] - a[0]}


def verify_draw_bindings(text):
    """Fresh explicit binding emission, not proof of a native shader/object path."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("draw-binding diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[draw-bindings]" in line and any(word in line for word in ("invalid", "refused", "unsupported")):
            raise ValueError("draw-binding submission failure")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = DRAW_BINDING_METRIC.search(line)
        if match:
            metrics.append((index, tuple(map(int, match.groups()))))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] - a[0] < 32 or b[1] <= a[1] or b[2] <= a[2]:
        raise Pending("need fresh emitted draws and explicit descriptor/layout binds")
    return dict(zip(("draws_delta", "descriptor_binds_delta", "layout_binds_delta"),
                    (b[i] - a[i] for i in range(3))))


def verify_model_nodes(text):
    """Fresh owned-node bounds use and source comparison, not a direct draw gate."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("model-node diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-model-node-mismatch]" in line:
            raise ValueError("native model-node bounds mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = MODEL_NODE_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[3]:
                raise ValueError("native model-node bounds differ from source")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] - a[0] < 32 or b[2] - a[2] < 32:
        raise Pending("need fresh owned node/bounds reads and checks")
    return {"reads_delta": b[0] - a[0], "checks_delta": b[2] - a[2], "unavailable": b[1]}


def verify_object_inputs(text):
    """Fresh owned color consumption/comparison; packet counts are not draws."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("object-input diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-object-input-mismatch]" in line:
            raise ValueError("native object input mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = OBJECT_INPUT_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[4]:
                raise ValueError("native object input differs from source")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or b[1] - a[1] < 32 or b[3] - a[3] != b[1] - a[1]:
        raise Pending("need fresh object publications and completely checked owned color reads")
    return {"publications_delta": b[0] - a[0], "reads_delta": b[1] - a[1],
            "checks_delta": b[3] - a[3], "unavailable": b[2], "packets_observed": b[5]}


def verify_selected_lights(text):
    """Fresh producer comparison and owned normal-lit consumption, not native GPU draws."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("selected-light diagnostic exceeds 400 KiB")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-selected-light-mismatch]" in line:
            raise ValueError("native selected light mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = SELECTED_LIGHT_METRIC.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[4] or values[8]:
                raise ValueError("selected light differs at publication or draw consumption")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if (any(b[i] < a[i] for i in range(len(b))) or
            any(b[i] - a[i] < 32 for i in (0, 5, 7)) or b[1] <= a[1] or
            b[3] - a[3] != b[0] - a[0]):
        raise Pending("need fresh completely checked light publications, changes, object snapshots and draw checks")
    return dict(zip(("publications_delta", "changed_slots_delta", "compatibility_delta",
                     "checks_delta", "snapshots_delta", "unavailable_delta", "draw_checks_delta"),
                    (b[i] - a[i] for i in (0, 1, 2, 3, 5, 6, 7))))


def verify_light_selection(text):
    """Fresh scored/rebuilt selections, complete original checks and no fallback growth."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("light-selection diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-light-selection\] (\d+) updates (\d+) rebuilds (\d+) candidates (\d+) compatibility; (\d+) checks wrong (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-light-selection-mismatch]" in line:
            raise ValueError("native light selection mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[5]:
                raise ValueError("light selection differs from original")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if (any(b[i] < a[i] for i in range(len(b))) or b[0] - a[0] < 32 or
            b[1] <= a[1] or b[2] <= a[2] or b[4] - a[4] != b[0] - a[0]):
        raise Pending("need fresh scored/rebuilt light selections with complete original comparisons")
    if b[3] != a[3]:
        raise ValueError("light selection used compatibility fallback in the field window")
    return dict(zip(("updates_delta", "rebuilds_delta", "candidates_delta", "checks_delta"),
                    (b[i] - a[i] for i in (0, 1, 2, 4))))


def verify_rigid_shadow(text):
    """Actual direct caster submission and fence retirement in fresh field windows.

    This is not source-free cold-load/reload or scene/receiver qualification.
    """
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("rigid shadow diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-rigid-shadow\] frame (\d+) submitted (\d+) suppressed (\d+) fence-retired (\d+); node (\d+) instance (\d+) generation (\d+) phase (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-rigid-shadow]" in line and "node refused:" in line:
            raise ValueError("direct rigid caster refused; no fallback qualification")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[4] != 64 or not values[5] or not values[6] or values[7] > 1:
                raise ValueError("unexpected rigid caster identity/phase")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if (b[0] <= a[0] or b[1] - a[1] < 32 or b[3] - a[3] < 32 or
            b[2] < a[2] or b[3] > b[1] or b[5:8] != a[5:8]):
        raise Pending("need fresh direct rigid shadow submissions and matching fence retirement")
    return {"submitted_delta": b[1] - a[1], "retired_delta": b[3] - a[3]}


def verify_rigid_scene(text):
    """Fresh direct scene command emission and fence lifetime, not reload proof."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("rigid scene diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-rigid-scene\] frame (\d+) submitted (\d+) emitted (\d+) suppressed (\d+) fence-retired (\d+); node (\d+) instance (\d+) generation (\d+);")
    visibility = re.compile(r"visibility pending (\d+) culled (\d+) retired-culled (\d+) resources-retired (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if re.search(r"\[native-rigid-scene\].*refused", line):
            raise ValueError("direct rigid scene refused; no fallback qualification")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[5] != 64 or not values[6] or not values[7] or values[2] > values[1] or values[4] > values[2]:
                raise ValueError("invalid rigid scene identity/emission/fence counts")
            classified = visibility.search(line)
            if "visibility pending" in line and not classified:
                raise ValueError("malformed native visibility classification")
            if classified:
                pending, culled, retired_culled, resources_retired = map(int,classified.groups())
                if (pending > 8192 or values[1] != values[2]+culled+pending or retired_culled > culled or
                        resources_retired != values[4]+retired_culled or resources_retired > values[1]):
                    raise ValueError("native submitted/visible/culled/pending/retired conservation failed")
                values += (pending,culled,retired_culled,resources_retired)
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if len(a) != len(b):
        raise ValueError("cannot join historical and GPU-classified scene evidence")
    if len(a) == 12 and any(b[i] < a[i] for i in (9,10,11)):
        raise ValueError("native GPU classification counters regressed")
    if (b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1,2,4)) or
            b[3] < a[3] or b[5:8] != a[5:8] or (len(a) == 8 and b[1]-a[1] != b[2]-a[2])):
        raise Pending("need fresh direct scene submissions, real draw emission and fence retirement")
    result = dict(submitted_delta=b[1]-a[1], emitted_delta=b[2]-a[2], retired_delta=b[4]-a[4])
    if len(a) == 12:
        result.update(pending=b[8],culled_delta=b[9]-a[9],retired_culled_delta=b[10]-a[10],resource_retired_delta=b[11]-a[11])
    return result


def verify_rigid_deferred(text, require_effects=False, require_inputs=False):
    """Fresh mixed consumer reachability; pixels and deferred-specific GPU retirement remain separate."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("native deferred diagnostic exceeds 400 KiB")
    if re.search(r"\[(?:error|critical)\]|\[native-[^\]]*-mismatch\]|\[shutdown\]", text):
        raise ValueError("runtime failure before native deferred qualification")
    native = re.compile(r"\[native-deferred\] frame (\d+) staged (\d+) consumed (\d+) pending (\d+);")
    effect = re.compile(r"\[native-deferred-effects\] frame (\d+) begins (\d+) ends (\d+) reads (\d+);")
    inputs = re.compile(r"\[native-deferred-inputs\] frame (\d+) batches (\d+) visuals (\d+) refreshes (\d+);")
    require_effects = require_effects or require_inputs
    receipt_size = 14 if require_inputs else 11 if require_effects else 8
    legacy = re.compile(r"\[host-consumer\] lists (\d+) entries (\d+) replayed \d+; direct draws (\d+) shells \d+ stencil \d+; "
                        r"bridges visual \d+ material (\d+) .*; fallback (\d+) refused (\d+)")
    contexts, metrics = [], []
    last_host, last_native_line, context_frame = None, -1, None
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
            match = re.search(r"\[native-material-context\] frame (\d+) ", line)
            context_frame = int(match[1]) if match else None
        host = legacy.search(line)
        if host:
            values = tuple(map(int, host.groups()))
            if values[-2:] != (0, 0):
                raise ValueError("mixed deferred consumer fell back or refused")
            last_host = index, values[:4]
        if require_inputs and "[native-deferred-inputs]" in line:
            match = inputs.search(line)
            if not match or not metrics or len(metrics[-1][1]) != 11:
                raise ValueError("native visual inputs lack their paired deferred/effect receipt")
            values = tuple(map(int, match.groups()))
            current = metrics[-1][1]
            previous = metrics[-2][1][11:14] if len(metrics) > 1 else (0, 0, 0)
            batches, visuals, refreshes = values[1:]
            batch_delta, visual_delta = batches - previous[0], visuals - previous[1]
            if (values[0] != current[0] or not 0 <= batches <= visuals <= current[2] or
                    batches > current[4] or batch_delta < 0 or refreshes < previous[2] or
                    (refreshes and not batches) or
                    not batch_delta <= visual_delta <= 4096 * batch_delta or
                    bool(visuals) != bool(current[2])):
                raise ValueError("native visual input frame, bounds or publication conservation failure")
            metrics[-1] = (index, current + values[1:])
            continue
        if require_effects and "[native-deferred-effects]" in line:
            match = effect.search(line)
            if not match or not metrics or len(metrics[-1][1]) != 8:
                raise ValueError("native visual effects lack their matching deferred receipt")
            values = tuple(map(int, match.groups()))
            current = metrics[-1][1]
            if (values[0] != current[0] or values[1] != values[2] or values[3] != current[2] or
                    values[1] > values[3] or (len(metrics) > 1 and any(
                        values[i] < metrics[-2][1][i+7] for i in (1,2,3)))):
                raise ValueError("native visual scope balance, frame or effect-read conservation failure")
            metrics[-1] = (index, current + values[1:])
            continue
        match = native.search(line)
        if "[native-deferred]" not in line:
            continue
        if not match or last_host is None or last_host[0] <= last_native_line:
            raise ValueError("native deferred receipt lacks its matching mixed consumer report")
        if metrics and len(metrics[-1][1]) != receipt_size:
            raise ValueError("native deferred receipt missing its visual publication")
        values = tuple(map(int, match.groups())) + last_host[1]
        if (values[1] != values[2] + values[3] or values[3] > 5140 or
                values[2] > values[5] or values[6] > values[5] or
                (contexts and (context_frame is None or values[0] < context_frame)) or
                (metrics and (values[0] <= metrics[-1][1][0] or any(
                    values[i] < metrics[-1][1][i] for i in (1, 2, 4, 5, 6, 7))))):
            raise ValueError("native deferred stale frame or counter conservation failure")
        metrics.append((index, values))
        last_native_line = index
    if metrics and len(metrics[-1][1]) != receipt_size:
        raise Pending("waiting for the matching native visual publication receipt")
    a, b = recent_field_samples(contexts, metrics)
    if a[3] or b[3] or any(b[i] - a[i] < 32 for i in (1, 2, 6, 7)):
        raise Pending("need fresh native and legacy consumption in consecutive ready-field windows")
    result = dict(first_frame=a[0], last_frame=b[0], staged_delta=b[1]-a[1],
                consumed_delta=b[2]-a[2], pending=b[3], legacy_draws_delta=b[6]-a[6],
                material_bridges_delta=b[7]-a[7])
    if require_effects:
        if any(b[i]-a[i] < 32 for i in (8,9,10)):
            raise Pending("need fresh paired native visual scopes and owned effect consumption")
        result.update(visual_begins_delta=b[8]-a[8], visual_ends_delta=b[9]-a[9], effects_delta=b[10]-a[10])
    if require_inputs:
        if any(b[i]-a[i] < 32 for i in (11,12,13)):
            raise Pending("need fresh native input batches, visual identities and completed-writer refreshes")
        result.update(input_batches_delta=b[11]-a[11], input_visuals_delta=b[12]-a[12], input_refreshes_delta=b[13]-a[13])
    return result


def verify_rigid_batches(text):
    """Native instance/indirect emissions; zero merged instances is explicit, not coverage of merging."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("rigid batch diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-rigid-batch\] frame (\d+) scene instances (\d+) (?:visible )?indirect calls (\d+) shadow instances (\d+) (?:visible )?indirect calls (\d+) merged instances (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-rigid-batch] refused" in line:
            raise ValueError("native batching refused; no translated fallback qualification")
        if "[native-material-context]" in line:
            contexts.append((index,line))
        match = metric.search(line)
        if match:
            values = tuple(map(int,match.groups()))
            if values[2] > values[1] or values[4] > values[3] or values[5] > values[1]+values[3]:
                raise ValueError("impossible native batch counts")
            metrics.append((index,values))
    a,b = recent_field_samples(contexts,metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in range(1,5)) or b[5] < a[5]:
        raise Pending("need fresh native scene/shadow instance and indirect emissions")
    return dict(scene_instances_delta=b[1]-a[1], scene_calls_delta=b[2]-a[2],
                shadow_instances_delta=b[3]-a[3], shadow_calls_delta=b[4]-a[4], merged_instances_delta=b[5]-a[5])


def verify_rigid_hard_off(text):
    """Pre-cull hard-off admission, not proof of teardown/reload or GPU output."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("rigid hard-off diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-rigid-hard-off\] frame (\d+) scene checks (\d+) shadow checks (\d+); node (\d+) generation (\d+);")
    contexts, metrics = [], []
    first_native = None
    for index, line in enumerate(text.splitlines()):
        if "[native-rigid-hard-off] refused:" in line:
            raise ValueError("hard-off native routing refused; fallback cannot qualify")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        if first_native is None and re.search(r"\[native-rigid-(?:scene|shadow)\] frame \d+ submitted [1-9]\d*", line):
            first_native = index
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if not values[4]:
                raise ValueError("hard-off native model generation is invalid")
            metrics.append((index, values))
    if not metrics or first_native is None or metrics[0][0] >= first_native:
        raise Pending("need hard-off admission before the first native submission")
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1, 2)) or b[3:] != a[3:]:
        raise Pending("need fresh scene/shadow admission in one loaded generation")
    return dict(scene_checks_delta=b[1]-a[1], shadow_checks_delta=b[2]-a[2])


def verify_shadow_images(text):
    """Require fresh native shadow completion and exact image-owner publication."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("shadow image diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-shadow-images\] begins (\d+) ends (\d+) publications (\d+); ownership checks (\d+) wrong (\d+); compatibility (\d+) (\d+); empty clears (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[4]:
                raise ValueError("native shadow image ownership mismatch")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if any(b[i] < a[i] for i in range(len(b))):
        raise Pending("shadow image counters reset")
    if b[5:7] != a[5:7]:
        raise ValueError("shadow lifecycle used compatibility in the field window")
    if any(b[i] - a[i] < 32 for i in range(4)) or b[3] - a[3] != b[2] - a[2]:
        raise Pending("need fresh completed native shadow passes with fully checked image handoffs")
    return dict(zip(("begins_delta", "ends_delta", "publications_delta", "checks_delta", "empty_clears_delta"),
                    (b[i] - a[i] for i in (0, 1, 2, 3, 7))))


def verify_fog(text):
    """Require active fog, fresh owned snapshots and complete producer checks."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("fog diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-fog\] (\d+) updates (\d+) inactive (\d+) compatibility (\d+) resets; (\d+) checks wrong (\d+); (\d+) object snapshots (\d+) unavailable; (\d+) draw checks (\d+) active layers wrong (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-fog-mismatch]" in line:
            raise ValueError("native fog mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[5] or values[10]:
                raise ValueError("fog differs at publication or draw consumption")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if (any(b[i] < a[i] for i in range(len(b))) or
            any(b[i] - a[i] < 32 for i in (0, 6, 8)) or b[9] <= a[9] or
            b[4] - a[4] != b[0] - a[0] + b[1] - a[1]):
        raise Pending("need fresh completely checked fog updates, owned snapshots and active-layer draw checks")
    return dict(zip(("updates_delta", "inactive_delta", "compatibility_delta", "resets_delta",
                     "checks_delta", "snapshots_delta", "unavailable_delta", "draw_checks_delta", "active_layers_delta"),
                    (b[i] - a[i] for i in (0, 1, 2, 3, 4, 6, 7, 8, 9))))


def verify_primitive_shader(text):
    """Fresh comparisons and actual consumption of owned primitive shader inputs."""
    return _verify_owned_shader_inputs(text, "native-primitive-shader")


def verify_lighting_pass(text):
    """Fresh pass-input comparisons plus consumption, never startup-only proof."""
    return _verify_owned_shader_inputs(text, "native-lighting-pass")


def verify_material_features(text):
    """Fresh ordered material/pass feature comparisons and actual consumption."""
    return _verify_owned_shader_inputs(text, "native-material-feature")


def verify_material_samplers(text):
    """Require fresh ordinary sampler comparisons and actual recipe consumption."""
    return _verify_owned_shader_inputs(text, "native-material-sampler")


def _verify_owned_shader_inputs(text, name):
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("owned shader diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[" + re.escape(name) + r"\] (\d+) checks wrong (\d+); (\d+) owned-input draws;")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if f"[{name}-mismatch]" in line:
            raise ValueError(f"{name} mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if values[1]:
                raise ValueError(f"{name} input differs from live draw")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] - a[0] < 32 or b[2] - a[2] < 32:
        raise Pending(f"need fresh {name} comparisons and owned-input draws")
    return dict(checks_delta=b[0] - a[0], draws_delta=b[2] - a[2])


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


def split_rigid_reload(text):
    """Validate one bounded same-process round trip; return independent epochs.

    Lifetime records are not pixels. Each returned <=400 KiB epoch must still
    pass the existing fresh-field/consumer checks; no across-load counter deltas.
    """
    if len(text.encode("utf-8")) > 2 * MAX_LOG_BYTES:
        raise ValueError("reload diagnostic exceeds 800 KiB")
    if re.search(r"\[(?:error|critical)\]|\[native-rigid-reload\] refused:", text):
        raise ValueError("runtime reload refused or reported an error")
    if "[shutdown] requested (guest-exit)" in text:
        raise ValueError("title-menu Exit ended the diagnostic; no continuing reload observation")
    complete = list(re.finditer(r"\[native-rigid-reload\] complete old-generation (\d+) new-generation (\d+) old-instance (\d+) new-instance (\d+);", text))
    if not complete:
        raise Pending("need completed same-process selected-asset reload")
    if len(complete) != 1:
        raise ValueError("multiple reload completions in one diagnostic")
    old_gen, new_gen, old_instance, new_instance = map(int, complete[0].groups())
    if not all((old_gen,new_gen,old_instance,new_instance)) or old_gen == new_gen or old_instance == new_instance:
        raise ValueError("reload did not produce fresh model and instance identities")
    pattern = re.compile(r"\[native-rigid-lifecycle\] (\S+) generation (\d+) instance (\d+) source-retired ([01]) scene (\d+)/(\d+)/(\d+) shadow (\d+)/(\d+)/(\d+);")
    rows = [(m.start(), m.end(), m[1], tuple(map(int,m.groups()[1:]))) for m in pattern.finditer(text)]
    expected = [("loaded",old_gen), ("cold-qualified",old_gen), ("source-retired",old_gen),
                ("closed-at-title",old_gen), ("loaded",new_gen), ("reload-qualified",new_gen)]
    if [(r[2],r[3][0]) for r in rows] != expected:
        raise ValueError("missing, duplicate or reordered selected source lifetime events")
    request = re.search(r"\[native-rigid-reload\] title requested generation (\d+) instance (\d+) task-uid ([1-9]\d*) sequence-id ([1-9]\d*)",text)
    if (not request or tuple(map(int,request.groups()[:2])) != (old_gen,old_instance) or
            not rows[1][1] < request.start() < rows[2][0] or complete[0].start() < rows[5][1]):
        raise ValueError("return-to-title request or completion ordering is invalid")
    for _, _, event, values in rows:
        gen, instance, retired, scene_s, scene_e, scene_r, shadow_s, shadow_e, shadow_r = values
        if scene_e > scene_s or scene_r > scene_s or shadow_e > shadow_s or shadow_r > shadow_s:
            raise ValueError("native output/retirement exceeds retained submissions")
        if event == "loaded":
            if any(values[1:]):
                raise ValueError("loaded generation inherited previous evidence")
        elif instance != (old_instance if gen == old_gen else new_instance):
            raise ValueError("selected instance identity changed in lifecycle evidence")
        if bool(retired) != (event in ("source-retired", "closed-at-title")):
            raise ValueError("source-retirement state disagrees with lifecycle")
        if event == "closed-at-title" and (scene_r != scene_s or shadow_r != shadow_s):
            raise ValueError("old native draws are still fence-retained at title")
    for earlier, later in ((1,2),(2,3)):
        if any(b < a for a,b in zip(rows[earlier][3][3:],rows[later][3][3:])):
            raise ValueError("old generation counters regressed")
    windows = list(re.finditer(r"\[native-rigid-reload\] window generation (\d+) scene (\d+)->(\d+) shadow (\d+)->(\d+)",text))
    if len(windows) != 2:
        raise ValueError("need independent interactive-field output windows")
    for window, row, end in zip(windows,(rows[1],rows[5]),(request.start(),complete[0].start())):
        generation, scene_before, scene_after, shadow_before, shadow_after = map(int,window.groups())
        if (generation != row[3][0] or scene_after != row[3][4] or shadow_after != row[3][7] or
                scene_after-scene_before < 900 or shadow_after-shadow_before < 900 or not row[1] < window.start() < end):
            raise ValueError("each loaded generation needs 900 fresh scene and shadow emissions")
    title = re.search(r"\[native-rigid-reload\] title reached; old generation (\d+) fully fence-retired; autoplay epoch 1",text)
    if not title or int(title[1]) != old_gen or not rows[3][1] < title.start() < rows[4][0]:
        raise ValueError("missing closed title epoch before reloading")
    cold, reloaded = text[:rows[1][0]], text[title.end():]
    if any(len(epoch.encode("utf-8")) > MAX_LOG_BYTES for epoch in (cold,reloaded)):
        raise ValueError("one reload field epoch exceeds 400 KiB")
    return cold,reloaded,dict(old_generation=old_gen,new_generation=new_gen,old_instance=old_instance,new_instance=new_instance)


def verify_receiver_setup(text):
    """Host callback and retained packet consumption, not source-free scene loading."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("receiver setup diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-shadow-receiver\] frame (\d+) native (\d+) ignored (\d+) original (\d+) refused (\d+); compatibility bindings (\d+) parameters (\d+); owned packets (\d+) reads (\d+) missing (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-shadow-receiver] refused" in line:
            raise ValueError("native receiver setup refused")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if match:
            values = tuple(map(int, match.groups()))
            if any(values[i] for i in (3, 4, 9)):
                raise ValueError("receiver original execution, refusal or missing owned packet")
            if values[2] > values[1] or values[6] > values[5] or values[7] > values[6]:
                raise ValueError("impossible receiver publication counts")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1, 5, 6, 7, 8)):
        raise Pending("need fresh host receiver setup, publication and native consumer reads")
    return dict(native_delta=b[1]-a[1], packets_delta=b[7]-a[7], reads_delta=b[8]-a[8], original=b[3])


def verify_scene_lights(text):
    """Fresh handoff-to-native selection with exact pass/instance identities."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("scene lighting diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-scene-lights\] frame (\d+) update (\d+) pass update (\d+) light view (\d+); (\d+) publications (\d+) refused; (\d+) bindings (\d+) unavailable imports; (\d+) native reads (\d+) missing; instance (\d+) generation (\d+) node (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if "[native-scene-lights]" in line and not match:
            raise ValueError("invalid or failed scene lighting handoff")
        if match:
            v = tuple(map(int, match.groups()))
            if not v[1] or v[1] != v[2] or v[3] >= 16 or not 0 < v[6] <= 65536 or v[9] or not v[10] or not v[11]:
                raise ValueError("scene light update/view/instance identity or native read failure")
            metrics.append((index, v))
    a, b = recent_field_samples(contexts, metrics)
    if b[5] != a[5]:
        raise ValueError("scene light publication refused in the field window")
    if b[0] <= a[0] or b[1] <= a[1] or b[4]-a[4] < 32 or b[8]-a[8] < 32 or b[10:13] != a[10:13]:
        raise Pending("need fresh scene light handoffs and native reads for the same field instance")
    return dict(updates_delta=b[4]-a[4], reads_delta=b[8]-a[8], bindings=b[6],
                unavailable_imports=b[7], generation=b[11], light_view=b[3])


def verify_caster_family(text):
    """Fresh multi-primitive caster work, excluding the selected reload asset."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("caster family diagnostic exceeds 400 KiB")
    metric = re.compile(r"\[native-caster-family\] frame (\d+) nodes (\d+) multi-primitive nodes (\d+) submitted (\d+) emitted (\d+) fence-retired (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if "[native-rigid-shadow]" in line and "node refused:" in line:
            raise ValueError("native caster refused; family not qualified")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if "[native-caster-family]" in line and not match:
            raise ValueError("malformed native caster family evidence")
        if match:
            values = tuple(map(int, match.groups()))
            if values[2] > values[1] or values[5] > values[4] or values[4] > values[3]:
                raise ValueError("impossible caster participation/emission/lifetime counts")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1, 2, 3, 4, 5)):
        raise Pending("need fresh non-regression multi-primitive casters, emissions and fence retirement")
    return dict(nodes_delta=b[1]-a[1], multi_nodes_delta=b[2]-a[2],
                submitted_delta=b[3]-a[3], emitted_delta=b[4]-a[4], retired_delta=b[5]-a[5])


def verify_cutout_family(text):
    """Fresh scene AND shadow cutouts, including textured emissions and fences."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("cutout family diagnostic exceeds 400 KiB")
    counts = r"submitted (\d+) emitted (\d+) fence-retired (\d+) textured-emitted (\d+) textured-retired (\d+);"
    metric = re.compile(r"\[native-cutout-family\] frame (\d+) scene " + counts + r" shadow " + counts)
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if re.search(r"\[native-rigid-(?:scene|shadow|batch)\].*refused", line):
            raise ValueError("native cutout consumer refused; family not qualified")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if "[native-cutout-family]" in line and not match:
            raise ValueError("malformed native cutout family evidence")
        if match:
            values = tuple(map(int, match.groups()))
            for start in (1, 6):
                submitted, emitted, retired, textured, textured_retired = values[start:start+5]
                if not (textured_retired <= textured <= emitted <= submitted and
                        textured_retired <= retired <= emitted):
                    raise ValueError("impossible cutout emission/lifetime counts")
            if metrics and any(b < a for a, b in zip(metrics[-1][1], values)):
                raise ValueError("cutout counters or frame reset inside a reload epoch")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in range(1, 11)):
        raise Pending("need fresh scene and shadow cutout emissions, textures and fence retirement")
    return {f"{view}_{name}_delta": b[start+i]-a[start+i]
            for view, start in (("scene", 1), ("shadow", 6))
            for i, name in enumerate(("submitted", "emitted", "retired", "textured_emitted", "textured_retired"))}


def verify_skin_shadow(text):
    return _verify_skin(text, "shadow", True)


def verify_skin_scene(text):
    return _verify_skin(text, "scene", False)


def verify_render_poses(text):
    """Fresh host interpolation and shared leases, not motion/pixel parity."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("render-pose diagnostic exceeds 400 KiB")
    tag = "[native-render-poses]"
    metric = re.compile(re.escape(tag) + r" frame (\d+) reads (\d+) blends (\d+) reused (\d+) snapped (\d+) refused (\d+);")
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if re.search(r"\[(?:error|critical)\]|\[native-instance-drift\]", line):
            raise ValueError("runtime failure or raw completed-pose mismatch")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if tag in line and not match:
            raise ValueError("malformed native render-pose evidence")
        if match:
            values = tuple(map(int, match.groups()))
            if values[5] or values[1] != sum(values[2:]):
                raise ValueError("render-pose refusal or inconsistent read outcomes")
            if metrics and any(b < a for a, b in zip(metrics[-1][1], values)):
                raise ValueError("render-pose counters or frame reset inside a reload epoch")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1, 2, 3)):
        raise Pending("need fresh native pose interpolation and shared render reads in the ready field")
    return dict(first_frame=a[0], last_frame=b[0], reads_delta=b[1]-a[1],
                blends_delta=b[2]-a[2], reused_delta=b[3]-a[3], snaps_delta=b[4]-a[4])


def _verify_skin(text, kind, identity):
    """Fresh native skin emission and fences; not pixel or per-character proof."""
    if len(text.encode("utf-8")) > MAX_LOG_BYTES:
        raise ValueError("skin diagnostic exceeds 400 KiB")
    tag = f"[native-skin-{kind}]"
    metric = re.compile(re.escape(tag) + r" frame (\d+) submitted (\d+) emitted (\d+) fence-retired (\d+);" +
                        (r" model (\d+) instance (\d+);" if identity else ""))
    contexts, metrics = [], []
    for index, line in enumerate(text.splitlines()):
        if re.search(r"\[(?:error|critical)\]|\[native-rigid-(?:shadow|scene|batch)\].*refused", line):
            raise ValueError("runtime failure or native skin consumer refusal")
        if "[native-material-context]" in line:
            contexts.append((index, line))
        match = metric.search(line)
        if tag in line and not match:
            raise ValueError("malformed native skin evidence")
        if match:
            values = tuple(map(int, match.groups()))
            if not (values[3] <= values[2] <= values[1]) or not all(values[4:]):
                raise ValueError("impossible skin emission/lifetime counts or missing identity")
            if metrics and any(b < a for a, b in zip(metrics[-1][1][:4], values[:4])):
                raise ValueError("skin counters or frame reset inside a reload epoch")
            metrics.append((index, values))
    a, b = recent_field_samples(contexts, metrics)
    if b[0] <= a[0] or any(b[i]-a[i] < 32 for i in (1, 2, 3)):
        raise Pending("need fresh native skin submissions, emissions and fence retirement")
    return dict(first_frame=a[0], last_frame=b[0], submitted_delta=b[1]-a[1],
                emitted_delta=b[2]-a[2], retired_delta=b[3]-a[3])


def verify_rigid_epoch(text, receiver_setup=False, scene_lights=False, caster_family=False, cutout_family=False,
                       rigid_deferred=False, deferred_effects=False, deferred_inputs=False, skin_shadow=False, skin_scene=False,
                       render_poses=False):
    verify(text)
    verify_texture_tables(text, comparison=False)
    verify_vertex_inputs(text, require_pulling=True)
    for check in (verify_movement,verify_canonical_geometry,verify_shadow_policies,
                  verify_material_textures,verify_primitive_policies,verify_lit_shading,
                  verify_draw_bindings,verify_model_nodes,verify_object_inputs,verify_selected_lights,
                  verify_light_selection,verify_shadow_images,verify_rigid_shadow,verify_rigid_scene,
                  verify_rigid_batches,verify_rigid_hard_off,verify_fog,verify_primitive_shader,
                  verify_lighting_pass,verify_material_features,verify_material_samplers):
        check(text)
    if receiver_setup:
        verify_receiver_setup(text)
    if scene_lights:
        verify_scene_lights(text)
    if caster_family:
        verify_caster_family(text)
    if cutout_family:
        verify_cutout_family(text)
    if skin_shadow:
        verify_skin_shadow(text)
    if skin_scene:
        verify_skin_scene(text)
    if render_poses:
        verify_render_poses(text)
    if rigid_deferred or deferred_effects or deferred_inputs:
        verify_rigid_deferred(text, require_effects=deferred_effects, require_inputs=deferred_inputs)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("log", type=Path)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--texture-tables", action="store_true")
    mode.add_argument("--texture-tables-normal", action="store_true")
    parser.add_argument("--vertex-inputs", action="store_true")
    parser.add_argument("--vertex-pulling", action="store_true")
    parser.add_argument("--canonical-geometry", action="store_true",
                        help="require fresh canonical draws and native vertex pulling")
    parser.add_argument("--movement", action="store_true")
    parser.add_argument("--shadow-policies", action="store_true")
    parser.add_argument("--material-textures", action="store_true")
    parser.add_argument("--primitive-policies", action="store_true")
    parser.add_argument("--lit-shading", action="store_true")
    parser.add_argument("--draw-bindings", action="store_true")
    parser.add_argument("--model-nodes", action="store_true")
    parser.add_argument("--object-inputs", action="store_true")
    parser.add_argument("--selected-lights", action="store_true")
    parser.add_argument("--light-selection", action="store_true")
    parser.add_argument("--shadow-images", action="store_true")
    parser.add_argument("--rigid-shadow", action="store_true")
    parser.add_argument("--rigid-scene", action="store_true")
    parser.add_argument("--rigid-deferred", action="store_true", help="fresh owned sorted packets and mixed legacy consumption; pixels separately required")
    parser.add_argument("--deferred-effects", action="store_true", help="also require paired native visual scopes and fresh owned effect reads")
    parser.add_argument("--deferred-inputs", action="store_true", help="also require bounded late native-identity input publications paired with native scopes")
    parser.add_argument("--rigid-batches", action="store_true")
    parser.add_argument("--rigid-hard-off", action="store_true")
    parser.add_argument("--rigid-reload", action="store_true", help="two independent field epochs and actual selected-source/fence retirement")
    parser.add_argument("--occlusion-observation", action="store_true",
                        help="standalone fresh-culling image trigger, NOT reload or pixel acceptance")
    parser.add_argument("--frame-probe", nargs=2, type=Path, metavar=("REQUEST", "JPEG"),
                        help="standalone bounded JPEG/frame/fence provenance check; pixels still need review")
    parser.add_argument("--receiver-setup", action="store_true", help="host receiver callback and fresh retained packet reads in each requested epoch")
    parser.add_argument("--scene-lights", action="store_true", help="owned scene/object handoffs and fresh native reads in each requested epoch")
    parser.add_argument("--caster-family", action="store_true", help="non-regression multi-primitive native caster emissions and fence retirement")
    parser.add_argument("--cutout-family", action="store_true", help="fresh scene/shadow cutout emissions and textured fence retirement in each requested epoch")
    parser.add_argument("--skin-shadow", action="store_true", help="fresh native skin emissions and fence retirement in each requested epoch; pixels separately required")
    parser.add_argument("--skin-scene", action="store_true", help="fresh native skin scene emissions and fence retirement in each requested epoch; pixels separately required")
    parser.add_argument("--render-poses", action="store_true", help="fresh native interpolation and shared pose reads in each requested epoch; pixels separately required")
    parser.add_argument("--fog", action="store_true")
    parser.add_argument("--primitive-shader", action="store_true")
    parser.add_argument("--lighting-pass", action="store_true")
    parser.add_argument("--material-features", action="store_true")
    parser.add_argument("--material-samplers", action="store_true")
    args = parser.parse_args()
    reload = None
    try:
        for standalone in ("occlusion_observation", "frame_probe"):
            if getattr(args, standalone) and any(value for name, value in vars(args).items()
                                                if name not in ("log", standalone)):
                raise ValueError("image observation is separate from acceptance switches")
        limit = MAX_LOG_BYTES * (2 if args.rigid_reload or args.occlusion_observation or args.frame_probe else 1)
        with args.log.open("rb") as source:
            data = source.read(limit + 1)
        if len(data) > limit:
            raise ValueError(f"instance diagnostic exceeds {limit // 1024} KiB")
        text = data.decode("utf-8")
        if args.frame_probe:
            from io import BytesIO
            from PIL import Image
            request_path, jpeg_path = args.frame_probe
            with request_path.open("rb") as source:
                request = source.read(65).decode("ascii")
            with jpeg_path.open("rb") as source:
                jpeg = source.read(110*1024+1)
            if not 0 < len(jpeg) <= 110*1024:
                raise ValueError("frame probe JPEG exceeds its byte limit")
            with Image.open(BytesIO(jpeg)) as image:
                if image.format != "JPEG" or not (0 < image.width <= 2048 and 0 < image.height <= 1200):
                    raise ValueError("frame probe must be a bounded, unresized JPEG")
                image.load()
                result = verify_frame_probe(text, request, len(jpeg), image.size)
            print("OBSERVED: renderer frame/fence provenance; pixels NOT qualified " +
                  ", ".join(f"{k}={v}" for k, v in result.items()))
            return 0
        if args.occlusion_observation:
            print("OBSERVED: fresh native culling; reload/pixels NOT qualified " +
                  ", ".join(f"{k}={v}" for k, v in observe_occlusion(text).items()))
            return 0
        if args.rigid_reload:
            cold, text, reload = split_rigid_reload(text)
            verify_rigid_epoch(cold, args.receiver_setup, args.scene_lights, args.caster_family, args.cutout_family, args.rigid_deferred, args.deferred_effects, args.deferred_inputs, args.skin_shadow, args.skin_scene, args.render_poses)
            verify_rigid_epoch(text, args.receiver_setup, args.scene_lights, args.caster_family, args.cutout_family, args.rigid_deferred, args.deferred_effects, args.deferred_inputs, args.skin_shadow, args.skin_scene, args.render_poses)
        result = verify(text)
        tables = verify_texture_tables(text, comparison=not args.texture_tables_normal) if (
            args.texture_tables or args.texture_tables_normal) else None
        vertex_inputs = verify_vertex_inputs(text, args.vertex_pulling or args.canonical_geometry) if (
            args.vertex_inputs or args.vertex_pulling or args.canonical_geometry) else None
        canonical = verify_canonical_geometry(text) if args.canonical_geometry else None
        movement = verify_movement(text) if args.movement else None
        shadow = verify_shadow_policies(text) if args.shadow_policies else None
        textures = verify_material_textures(text) if args.material_textures else None
        policies = verify_primitive_policies(text) if args.primitive_policies else None
        lit = verify_lit_shading(text) if args.lit_shading else None
        bindings = verify_draw_bindings(text) if args.draw_bindings else None
        nodes = verify_model_nodes(text) if args.model_nodes else None
        objects = verify_object_inputs(text) if args.object_inputs else None
        lights = verify_selected_lights(text) if args.selected_lights else None
        selection = verify_light_selection(text) if args.light_selection else None
        shadow_images = verify_shadow_images(text) if args.shadow_images else None
        rigid_shadow = verify_rigid_shadow(text) if args.rigid_shadow else None
        rigid_scene = verify_rigid_scene(text) if args.rigid_scene or args.rigid_deferred or args.deferred_effects or args.deferred_inputs else None
        rigid_deferred = verify_rigid_deferred(text, require_effects=args.deferred_effects, require_inputs=args.deferred_inputs) if args.rigid_deferred or args.deferred_effects or args.deferred_inputs else None
        rigid_batches = verify_rigid_batches(text) if args.rigid_batches else None
        rigid_hard_off = verify_rigid_hard_off(text) if args.rigid_hard_off else None
        receiver_setup = verify_receiver_setup(text) if args.receiver_setup else None
        scene_lights = verify_scene_lights(text) if args.scene_lights else None
        caster_family = verify_caster_family(text) if args.caster_family else None
        cutout_family = verify_cutout_family(text) if args.cutout_family else None
        skin_shadow = verify_skin_shadow(text) if args.skin_shadow else None
        skin_scene = verify_skin_scene(text) if args.skin_scene else None
        render_poses = verify_render_poses(text) if args.render_poses else None
        fog = verify_fog(text) if args.fog else None
        primitive_shader = verify_primitive_shader(text) if args.primitive_shader else None
        lighting_pass = verify_lighting_pass(text) if args.lighting_pass else None
        material_features = verify_material_features(text) if args.material_features else None
        material_samplers = verify_material_samplers(text) if args.material_samplers else None
    except Pending as error:
        print(f"Pending: {error}")
        return 2
    except (ValueError, OSError) as error:
        print(f"FAIL: {error}")
        return 1
    print("PASS: post-event native instances " + ", ".join(f"{k}={v}" for k, v in result.items()))
    if reload is not None:
        print("PASS: same-process native rigid reload (pixels separately required) " + ", ".join(f"{k}={v}" for k,v in reload.items()))
    if receiver_setup is not None:
        print("PASS: host receiver setup and owned reads " + ", ".join(f"{k}={v}" for k,v in receiver_setup.items()))
    if scene_lights is not None:
        print("PASS: owned scene/object lighting and native reads " + ", ".join(f"{k}={v}" for k,v in scene_lights.items()))
    if caster_family is not None:
        print("PASS: multi-primitive native caster family " + ", ".join(f"{k}={v}" for k,v in caster_family.items()))
    if cutout_family is not None:
        print("PASS: native scene/shadow cutout family (pixels separately required) " + ", ".join(f"{k}={v}" for k,v in cutout_family.items()))
    if skin_shadow is not None:
        print("PASS: native skin shadow emission and fences (pixels separately required) " + ", ".join(f"{k}={v}" for k,v in skin_shadow.items()))
    if skin_scene is not None:
        print("PASS: native skin scene emission and fences (pixels separately required) " + ", ".join(f"{k}={v}" for k,v in skin_scene.items()))
    if render_poses is not None:
        print("PASS: native render-pose interpolation and shared reads (pixels separately required) " + ", ".join(f"{k}={v}" for k,v in render_poses.items()))
    if tables is not None:
        print("PASS: post-event native texture tables " + ", ".join(f"{k}={v}" for k, v in tables.items()))
    if vertex_inputs is not None:
        print("PASS: post-event native vertex inputs " + ", ".join(f"{k}={v}" for k, v in vertex_inputs.items()))
    if canonical is not None:
        print("PASS: post-event canonical geometry " + ", ".join(f"{k}={v}" for k, v in canonical.items()))
    if movement is not None:
        print("PASS: post-event observed player movement " + ", ".join(f"{k}={v}" for k, v in movement.items()))
    if shadow is not None:
        print("PASS: post-event owned shadow policies " + ", ".join(f"{k}={v}" for k, v in shadow.items()))
    if textures is not None:
        print("PASS: post-event native material textures " + ", ".join(f"{k}={v}" for k, v in textures.items()))
    if policies is not None:
        print("PASS: post-event native primitive policies " + ", ".join(f"{k}={v}" for k, v in policies.items()))
    if lit is not None:
        print("PASS: post-event named lit shading " + ", ".join(f"{k}={v}" for k, v in lit.items()))
    if bindings is not None:
        print("PASS: post-event explicit draw bindings " + ", ".join(f"{k}={v}" for k, v in bindings.items()))
    if nodes is not None:
        print("PASS: post-event owned model nodes " + ", ".join(f"{k}={v}" for k, v in nodes.items()))
    if objects is not None:
        print("PASS: post-event owned object inputs " + ", ".join(f"{k}={v}" for k, v in objects.items()))
    if lights is not None:
        print("PASS: post-event owned selected lights " + ", ".join(f"{k}={v}" for k, v in lights.items()))
    if selection is not None:
        print("PASS: post-event host light selection " + ", ".join(f"{k}={v}" for k, v in selection.items()))
    if shadow_images is not None:
        print("PASS: post-event native shadow images " + ", ".join(f"{k}={v}" for k, v in shadow_images.items()))
    if rigid_shadow is not None:
        print("PASS: post-event direct rigid shadows " + ", ".join(f"{k}={v}" for k, v in rigid_shadow.items()))
    if rigid_scene is not None:
        print("PASS: post-event direct rigid scene " + ", ".join(f"{k}={v}" for k, v in rigid_scene.items()))
    if rigid_deferred is not None:
        print("PASS: post-event mixed native deferred consumer (pixels separately required) " + ", ".join(f"{k}={v}" for k, v in rigid_deferred.items()))
    if rigid_batches is not None:
        print("PASS: post-event native rigid batches " + ", ".join(f"{k}={v}" for k, v in rigid_batches.items()))
    if rigid_hard_off is not None:
        print("PASS: pre-cull hard-off rigid routing (admission-only check) " + ", ".join(f"{k}={v}" for k, v in rigid_hard_off.items()))
    if fog is not None:
        print("PASS: post-event owned fog " + ", ".join(f"{k}={v}" for k, v in fog.items()))
    if primitive_shader is not None:
        print("PASS: post-event owned primitive shader " + ", ".join(f"{k}={v}" for k, v in primitive_shader.items()))
    if lighting_pass is not None:
        print("PASS: post-event owned lighting pass " + ", ".join(f"{k}={v}" for k, v in lighting_pass.items()))
    if material_features is not None:
        print("PASS: post-event owned material features " + ", ".join(f"{k}={v}" for k, v in material_features.items()))
    if material_samplers is not None:
        print("PASS: post-event owned material samplers " + ", ".join(f"{k}={v}" for k, v in material_samplers.items()))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
