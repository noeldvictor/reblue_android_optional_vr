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
    parser.add_argument("--fog", action="store_true")
    parser.add_argument("--primitive-shader", action="store_true")
    parser.add_argument("--lighting-pass", action="store_true")
    parser.add_argument("--material-features", action="store_true")
    parser.add_argument("--material-samplers", action="store_true")
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
