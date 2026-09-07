import unittest
import re
from native_instance_scenario import verify_model_nodes, verify_object_inputs, verify_selected_lights
from native_instance_scenario import verify_fog, verify_primitive_shader, verify_lighting_pass, verify_material_features
from native_instance_scenario import verify_material_samplers, verify_light_selection, verify_shadow_images
from native_instance_scenario import verify_rigid_shadow
from native_instance_scenario import verify_rigid_scene
from native_instance_scenario import verify_rigid_batches
from native_instance_scenario import verify_rigid_hard_off
from native_instance_scenario import split_rigid_reload
from native_instance_scenario import verify_receiver_setup
from native_instance_scenario import verify_scene_lights
from native_instance_scenario import verify_caster_family
from native_instance_scenario import verify_cutout_family
from native_instance_scenario import (
    MAX_LOG_BYTES, Pending, READY, verify, verify_texture_tables,
    verify_vertex_inputs, verify_movement, verify_canonical_geometry, verify_shadow_policies,
    verify_material_textures, verify_primitive_policies, verify_lit_shading, verify_draw_bindings,
)


def metric(reads=100, checks=100, wrong=0, refused=0):
    return (f"[native-instances] 12 created 1 retired 11 live / 9000 bytes; "
            f"20 poses published 50 reused; 70 producer imports {refused} refused; "
            f"{reads} consumer reads 3 unavailable; {checks} checks wrong {wrong};")


def scenario():
    return ["[native-material-context] stage bg41_01 player 1 event 1 movie 0",
            "[native-material-context] " + READY, metric(),
            "[native-material-context] " + READY, metric(150, 140)]


def movement(t=40, episode=1, walking=1, duration=2, moved=30, distance=1.5):
    return (f"[autoplay] t {t:.3f} stage bg41_01 ready 1 walking {walking} episode {episode} "
            f"walk-s {duration:.3f} moved {moved} distance {distance:.6f} position 1.0,2.0,3.0")


class MovementScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = movement()
        rows[4] = movement(t=45, duration=7, moved=80, distance=4)
        return rows

    def test_fresh_displacement(self):
        result = verify_movement("\n".join(self.rows()))
        self.assertEqual(result["samples_delta"], 50)
        self.assertEqual(result["distance_delta"], 2.5)

    def test_stick_and_time_without_displacement_do_not_pass(self):
        for moved, distance in ((30, 4), (80, 1.5), (80, 1.51)):
            rows = self.rows()
            rows[-1] = movement(t=45, duration=7, moved=moved, distance=distance)
            with self.assertRaises(Pending):
                verify_movement("\n".join(rows))

    def test_reload_pause_and_new_episode_do_not_join(self):
        for last in (movement(t=45, episode=2, duration=7, moved=80, distance=4),
                     movement(t=45, walking=0, duration=7, moved=80, distance=4)):
            rows = self.rows(); rows[-1] = last
            with self.assertRaises(Pending):
                verify_movement("\n".join(rows))
            with self.assertRaises(Pending):
                verify_movement("\n".join(self.rows() + [last]))

    def test_ready_state_stage_and_observation_order(self):
        good = "\n".join(self.rows())
        for bad in (good.replace("ready 1", "ready 0"),
                    good.replace("stage bg41_01", "stage bg42_01"),
                    good.replace("t 45.000", "t 39.000"),
                    good.replace("walk-s 7.000", "walk-s 2.500"),
                    "\n".join(self.rows()[1:]),
                    "\n".join([movement()] + [r for r in self.rows() if "context" in r])):
            with self.assertRaises(Pending):
                verify_movement(bad)

    def test_invalid_and_bounded_observations(self):
        for bad in ("x" * (MAX_LOG_BYTES + 1),
                    "\n".join(self.rows()).replace("1.0,2.0,3.0", "nan,2.0,3.0")):
            with self.assertRaises(ValueError):
                verify_movement(bad)


class InstanceScenarioTest(unittest.TestCase):
    def test_fresh_field(self):
        result = verify("\n".join(scenario()))
        self.assertEqual(result["reads_delta"], 50)
        self.assertEqual(result["checks_delta"], 40)

    def test_startup_only(self):
        rows = scenario()
        with self.assertRaises(Pending):
            verify("\n".join([metric(1000, 1000)] + [r for r in rows if "context" in r]))

    def test_stale_checks_or_reads(self):
        for reads, checks in ((150, 100), (100, 140), (50, 40)):
            rows = scenario(); rows[-1] = metric(reads, checks)
            with self.assertRaises(Pending):
                verify("\n".join(rows))

    def test_wrong_or_refused_cannot_be_hidden_by_later_success(self):
        for bad in (metric(wrong=1), metric(refused=1)):
            with self.assertRaises(ValueError):
                verify("\n".join([bad] + scenario()))

    def test_wrong_scene_and_missing_event(self):
        rows = scenario()
        for text in ("\n".join(rows[1:]), "\n".join(rows).replace("field-state 0", "field-state 4")):
            with self.assertRaises(Pending):
                verify(text)

    def test_sample_before_context_is_not_current(self):
        rows = scenario(); rows[-2:] = reversed(rows[-2:])
        with self.assertRaises(Pending):
            verify("\n".join(rows))

    def test_bounded_input(self):
        with self.assertRaises(ValueError):
            verify("x" * (MAX_LOG_BYTES + 1))

    def test_unused_native_path_fails_in_ready_field_instead_of_waiting(self):
        rows = scenario()
        rows[-1] = metric(0, 0).replace("3 unavailable", "10000 unavailable")
        with self.assertRaisesRegex(ValueError, "no consumers"):
            verify("\n".join(rows))

    def test_later_context_does_not_hide_complete_fresh_windows(self):
        rows = scenario() + ["[native-material-context] " + READY]
        self.assertEqual(verify("\n".join(rows))["checks_delta"], 40)

    def test_interleaving_cannot_hide_a_scene_change_or_stale_samples(self):
        for extra in (["[native-material-context] mode Loading"],
                      ["[native-material-context] " + READY] * 2):
            with self.assertRaises(Pending):
                verify("\n".join(scenario() + extra))

    def test_missing_middle_window_cannot_join_nonconsecutive_samples(self):
        rows = scenario()
        rows.insert(-1, "[native-material-context] " + READY)
        with self.assertRaises(Pending):
            verify("\n".join(rows))


def table_metric(reads=100, images=100, wrong=0, image_wrong=0, refused=0):
    return (f"[native-texture-tables] 5 published 1 retired 4 indexed / 2000 bytes; "
            f"2 replacements {refused} refused; {reads} lookups 7 fallback; {reads} checks wrong {wrong}; "
            f"{images} image checks wrong {image_wrong}; 0 native image reads 0 unavailable;")


class TextureTableScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2], rows[4] = table_metric(), table_metric(150, 140)
        return rows

    def test_fresh_nonnull_images(self):
        result = verify_texture_tables("\n".join(self.rows()))
        self.assertEqual(result["lookups_delta"], 50)
        self.assertEqual(result["image_checks_delta"], 40)
        self.assertEqual(result["native_image_reads_delta"], 0)  # not falsely qualified

    def test_any_failed_publication_or_comparison_is_fatal(self):
        for bad in (table_metric(wrong=1), table_metric(image_wrong=1), table_metric(refused=1)):
            with self.assertRaises(ValueError):
                verify_texture_tables("\n".join([bad] + self.rows()))

    def test_null_only_or_stale_images_do_not_qualify(self):
        for images in (0, 100):
            rows = self.rows(); rows[-1] = table_metric(150, images)
            with self.assertRaises(Pending):
                verify_texture_tables("\n".join(rows))

    def test_unexercised_publisher_fails_after_ready_field_lookups(self):
        rows = self.rows()
        rows[-1] = table_metric(0, 0).replace("7 fallback", "10000 fallback")
        with self.assertRaisesRegex(ValueError, "no field consumers"):
            verify_texture_tables("\n".join(rows))

    def test_startup_wrong_scene_and_nonconsecutive_samples(self):
        rows = self.rows()
        for bad in ([table_metric(1000, 1000)] + [r for r in rows if "context" in r],
                    rows[:-1] + ["[native-material-context] " + READY, rows[-1]],
                    [r.replace("field-state 0", "field-state 4") for r in rows]):
            with self.assertRaises(Pending):
                verify_texture_tables("\n".join(bad))

    def test_bounded_and_interleaved(self):
        with self.assertRaises(ValueError):
            verify_texture_tables("x" * (MAX_LOG_BYTES + 1))
        self.assertEqual(verify_texture_tables("\n".join(self.rows() + [
            "[native-material-context] " + READY]))["image_checks_delta"], 40)

    def test_normal_mode_requires_fresh_reads_without_original_calls(self):
        rows = self.rows()
        rows = [re.sub(r"\d+ checks wrong", "0 checks wrong", row).replace(
            "7 fallback", "0 fallback") for row in rows]
        rows = [re.sub(r"\d+ image checks", "0 image checks", row) for row in rows]
        result = verify_texture_tables("\n".join(rows), comparison=False)
        self.assertEqual(result["lookups_delta"], 50)
        self.assertEqual(result["checks_delta"], 0)
        for bad in ("\n".join(rows).replace("0 fallback", "1 fallback"),
                    "\n".join(self.rows())):
            with self.assertRaises(ValueError):
                verify_texture_tables(bad, comparison=False)
        with self.assertRaises(Pending):
            verify_texture_tables("\n".join(rows).replace("150 lookups", "100 lookups"), comparison=False)

class VertexInputScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-vertex-input-use] 100 pipeline binds, 100 decode blocks, 0 pulled records"
        rows[4] = "[native-vertex-input-use] 150 pipeline binds, 140 decode blocks, 0 pulled records"
        return rows

    def test_fresh_consumers_do_not_claim_pulling_coverage(self):
        result = verify_vertex_inputs("\n".join(self.rows()))
        self.assertEqual(result, dict(pipeline_binds_delta=50, decode_blocks_delta=40, pulled_records_delta=0))

    def test_stale_or_missing_consumer(self):
        for old, new in (("140 decode", "100 decode"), ("150 pipeline", "0 pipeline")):
            with self.assertRaises(Pending):
                verify_vertex_inputs("\n".join(self.rows()).replace(old, new))

    def test_pulling_requires_positive_fresh_coverage(self):
        with self.assertRaises(Pending):
            verify_vertex_inputs("\n".join(self.rows()), require_pulling=True)
        rows = self.rows()
        rows[-1] = rows[-1].replace("0 pulled records", "50 pulled records")
        self.assertEqual(verify_vertex_inputs("\n".join(rows), require_pulling=True)["pulled_records_delta"], 50)

    def test_startup_wrong_scene_and_nonconsecutive_samples(self):
        rows = self.rows()
        for text in ("\n".join(rows).replace("field-state 0", "field-state 4"),
                     "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r]),
                     "\n".join(rows[:-1] + ["[native-material-context] " + READY, rows[-1]])):
            with self.assertRaises(Pending):
                verify_vertex_inputs(text)

    def test_bounded_and_interleaved(self):
        with self.assertRaises(ValueError):
            verify_vertex_inputs("x" * (MAX_LOG_BYTES + 1))
        self.assertEqual(verify_vertex_inputs("\n".join(self.rows() + [
            "[native-material-context] " + READY]))["pipeline_binds_delta"], 50)


class CanonicalGeometryScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-mesh-canonical] 12 meshes, 100 draws, 0 source-free disk loads;"
        rows[4] = "[native-mesh-canonical] 12 meshes, 150 draws, 0 source-free disk loads;"
        return rows

    def test_fresh_draws_do_not_claim_source_free_loading(self):
        self.assertEqual(verify_canonical_geometry("\n".join(self.rows())),
                         dict(meshes=12, draws_delta=50, source_free_loads_delta=0))

    def test_missing_or_unused_canonical_owners(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("12 meshes", "0 meshes"),
                    text.replace("150 draws", "100 draws"),
                    text.replace("150 draws", "131 draws"),
                    "\n".join(scenario())):
            with self.assertRaises(Pending):
                verify_canonical_geometry(bad)

    def test_counter_reset_is_not_fresh_activity(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150 draws", "10 draws"),
                    text.replace("12 meshes, 150", "11 meshes, 150"),
                    text.replace("100 draws, 0", "100 draws, 2")):
            with self.assertRaises(Pending):
                verify_canonical_geometry(bad)

    def test_startup_wrong_scene_missing_event_and_nonconsecutive_samples(self):
        rows = self.rows()
        for bad in ("\n".join(rows[1:]),
                    "\n".join(rows).replace("field-state 0", "field-state 4"),
                    "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r]),
                    "\n".join(rows[:-1] + ["[native-material-context] " + READY, rows[-1]])):
            with self.assertRaises(Pending):
                verify_canonical_geometry(bad)

    def test_interleaving_and_lost_readiness(self):
        rows = self.rows() + ["[native-material-context] " + READY]
        self.assertEqual(verify_canonical_geometry("\n".join(rows))["draws_delta"], 50)
        for extra in ("[native-material-context] mode Loading", "[native-material-context] " + READY):
            with self.assertRaises(Pending):
                verify_canonical_geometry("\n".join(rows + [extra]))

    def test_bounded_input(self):
        with self.assertRaises(ValueError):
            verify_canonical_geometry("x" * (MAX_LOG_BYTES + 1))


class ShadowPolicyScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = ("[native-model-shadow] 12 load-owned policies (2 disabled), 0 unknown; "
                   "100 draw lookups hit / 0 unavailable;\n"
                   "[native-shadow] receiver inputs checked 100 wrong 0 receiving 80; "
                   "replays composed 150 changed 20")
        rows[4] = rows[2].replace("100", "150").replace("receiving 80", "receiving 120").replace(
            "composed 150", "composed 200")
        return rows

    def test_fresh_owned_policy_and_receiver(self):
        result = verify_shadow_policies("\n".join(self.rows()))
        self.assertEqual(result["lookups_delta"], 50)
        self.assertEqual(result["checks_delta"], 50)
        self.assertEqual(result["receiving_delta"], 40)

    def test_each_consumer_must_advance(self):
        text = "\n".join(self.rows())
        for old, new in (("150 draw", "100 draw"), ("checked 150", "checked 100"),
                         ("receiving 120", "receiving 80"), ("composed 200", "composed 150"),
                         ("12 load-owned", "0 load-owned")):
            with self.assertRaises(Pending):
                verify_shadow_policies(text.replace(old, new))

    def test_mismatch_cannot_be_hidden_by_success(self):
        with self.assertRaises(ValueError):
            verify_shadow_policies("[native-shadow] receiver inputs checked 1 wrong 1 receiving 1; "
                                   "replays composed 1 changed 0\n" + "\n".join(self.rows()))

    def test_startup_and_cross_window_policy_are_not_field_proof(self):
        rows = self.rows()
        for bad in ("\n".join(rows[1:]), "\n".join(rows).replace("field-state 0", "field-state 4"),
                    "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r]),
                    "\n".join(rows[:-1] + [rows[-1].split("\n")[1]])):
            with self.assertRaises(Pending):
                verify_shadow_policies(bad)

    def test_bounded_input_and_counter_reset(self):
        with self.assertRaises(ValueError):
            verify_shadow_policies("x" * (MAX_LOG_BYTES + 1))
        with self.assertRaises(Pending):
            verify_shadow_policies("\n".join(self.rows()).replace("150 draw", "10 draw"))


class MaterialTextureScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = ("[native-material-textures] 100 object publications 0 with overrides; "
                   "3 unsupported 0 refused; 10 meshes prepared, peak 9000 bytes; "
                   "100 reads 3 unavailable; 100 checks wrong 0; 100 draws 100 image slots 100 UV blocks;")
        rows[4] = rows[2].replace("100", "150")
        return rows

    def test_fresh_consumers_do_not_claim_animated_override_coverage(self):
        result = verify_material_textures("\n".join(self.rows()))
        for name in ("publications_delta", "reads_delta", "checks_delta", "draws_delta",
                     "image_slots_delta", "uv_blocks_delta"):
            self.assertEqual(result[name], 50)
        self.assertEqual(result["override_publications_delta"], 0)

    def test_each_producer_and_consumer_must_advance(self):
        text = "\n".join(self.rows())
        for name in ("object publications", "reads", "checks", "draws", "image slots", "UV blocks"):
            with self.assertRaises(Pending):
                verify_material_textures(text.replace("150 " + name, "100 " + name))

    def test_failures_cannot_be_hidden_by_later_success(self):
        text = "\n".join(self.rows())
        for bad in (self.rows()[2].replace("0 refused", "1 refused"),
                    self.rows()[2].replace("wrong 0", "wrong 1"),
                    self.rows()[2].replace("9000 bytes", "4194305 bytes"),
                    "[native-material-texture-mismatch] visual 12345678 channel 16"):
            with self.assertRaises(ValueError):
                verify_material_textures(bad + "\n" + text)

    def test_startup_wrong_scene_and_cross_window_counters_do_not_qualify(self):
        rows = self.rows()
        for bad in ("\n".join(rows[1:]), "\n".join(rows).replace("field-state 0", "field-state 4"),
                    "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r]),
                    "\n".join(rows[:-1] + ["[native-material-context] " + READY, rows[-1]])):
            with self.assertRaises(Pending):
                verify_material_textures(bad)

    def test_bounded_reset_and_lost_readiness(self):
        with self.assertRaises(ValueError):
            verify_material_textures("x" * (MAX_LOG_BYTES + 1))
        for bad in ("\n".join(self.rows()).replace("150 draws", "10 draws"),
                    "\n".join(self.rows() + ["[native-material-context] mode Loading"])):
            with self.assertRaises(Pending):
                verify_material_textures(bad)
        self.assertEqual(verify_material_textures("\n".join(self.rows() + [
            "[native-material-context] " + READY]))["draws_delta"], 50)


class PrimitivePolicyScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = ("[native-primitive-policy] 100 plans 90 known 10 unknown; "
                   "100 direct 0 deferred 0 suppressed candidates; "
                   "100 reads 3 unavailable; 100 checks wrong 0; "
                   "100 draws 0 cull changes 0 compound refreshes;")
        rows[4] = rows[2].replace("100", "150").replace("90 known", "140 known")
        return rows

    def test_fresh_consumers_do_not_claim_unexercised_routing(self):
        result = verify_primitive_policies("\n".join(self.rows()))
        for name in ("plans_delta", "known_delta", "reads_delta", "checks_delta", "draws_delta"):
            self.assertEqual(result[name], 50)
        for name in ("deferred_candidates_delta", "suppressed_candidates_delta",
                     "cull_changes_delta", "compound_refreshes_delta"):
            self.assertEqual(result[name], 0)

    def test_each_producer_and_consumer_must_advance(self):
        text = "\n".join(self.rows())
        for old, new in (("150 plans", "100 plans"), ("140 known", "90 known"),
                         ("150 reads", "100 reads"), ("150 checks", "100 checks"),
                         ("150 draws", "100 draws")):
            with self.assertRaises(Pending):
                verify_primitive_policies(text.replace(old, new))

    def test_failures_cannot_be_hidden_by_later_success(self):
        for bad in (self.rows()[2].replace("wrong 0", "wrong 1"),
                    "[native-primitive-policy-mismatch] winding/participation"):
            with self.assertRaises(ValueError):
                verify_primitive_policies(bad + "\n" + "\n".join(self.rows()))

    def test_startup_wrong_scene_and_cross_window_counters_do_not_qualify(self):
        rows = self.rows()
        for bad in ("\n".join(rows[1:]), "\n".join(rows).replace("field-state 0", "field-state 4"),
                    "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r]),
                    "\n".join(rows[:-1] + ["[native-material-context] " + READY, rows[-1]])):
            with self.assertRaises(Pending):
                verify_primitive_policies(bad)

    def test_bounded_reset_and_lost_readiness(self):
        with self.assertRaises(ValueError):
            verify_primitive_policies("x" * (MAX_LOG_BYTES + 1))
        for bad in ("\n".join(self.rows()).replace("150 draws", "10 draws"),
                    "\n".join(self.rows() + ["[native-material-context] mode Loading"])):
            with self.assertRaises(Pending):
                verify_primitive_policies(bad)
        self.assertEqual(verify_primitive_policies("\n".join(self.rows() + [
            "[native-material-context] " + READY]))["draws_delta"], 50)


class LitShadingScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-lit-shading] 100 normal-lit queued draws;"
        rows[4] = "[native-lit-shading] 150 normal-lit queued draws;"
        return rows

    def test_fresh_queued_use(self):
        self.assertEqual(verify_lit_shading("\n".join(self.rows())), dict(queued_draws_delta=50))

    def test_missing_stale_reset_or_startup_is_not_coverage(self):
        rows = self.rows()
        for bad in ("\n".join(scenario()), "\n".join(rows).replace("150", "100"),
                    "\n".join(rows).replace("150", "10"), "\n".join(rows[1:]),
                    "\n".join([rows[2], rows[4]] + [r for r in rows if "context" in r])):
            with self.assertRaises(Pending):
                verify_lit_shading(bad)

    def test_bounded_and_lost_readiness(self):
        with self.assertRaises(ValueError):
            verify_lit_shading("x" * (MAX_LOG_BYTES + 1))
        with self.assertRaises(Pending):
            verify_lit_shading("\n".join(self.rows() + ["[native-material-context] mode Loading"]))
        self.assertEqual(verify_lit_shading("\n".join(self.rows() + [
            "[native-material-context] " + READY]))["queued_draws_delta"], 50)


class DrawBindingScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[draw-bindings] 100 emitted draws 120 descriptor binds 5 layout binds;"
        rows[4] = "[draw-bindings] 150 emitted draws 180 descriptor binds 9 layout binds;"
        return rows

    def test_fresh_emission(self):
        self.assertEqual(verify_draw_bindings("\n".join(self.rows())),
                         dict(draws_delta=50, descriptor_binds_delta=60, layout_binds_delta=4))

    def test_stale_reset_unused_startup_and_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("150", "1"),
                    text.replace("180", "120"), text.replace("9 layout", "5 layout"),
                    text.replace("bg41_01", "bg42_01"), "\n".join(self.rows()[1:]),
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_draw_bindings(bad)

    def test_refusals_cannot_be_hidden_and_input_is_bounded(self):
        for failure in ("invalid queued binding snapshot", "refused producer", "unsupported instance record ABI"):
            with self.assertRaises(ValueError):
                verify_draw_bindings("[draw-bindings] " + failure + "\n" + "\n".join(self.rows()))
        with self.assertRaises(ValueError):
            verify_draw_bindings("x" * (MAX_LOG_BYTES + 1))
        with self.assertRaises(Pending):
            verify_draw_bindings("\n".join(self.rows() + ["[native-material-context] mode Loading"]))


class ModelNodeScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-model-nodes] 100 owned bounds/primitive associations 5 unavailable; 100 bounds checks wrong 0;"
        rows[4] = "[native-model-nodes] 150 owned bounds/primitive associations 8 unavailable; 145 bounds checks wrong 0;"
        return rows

    def test_fresh_owned_use(self):
        self.assertEqual(verify_model_nodes("\n".join(self.rows())),
                         dict(reads_delta=50, checks_delta=45, unavailable=8))

    def test_stale_unused_reset_startup_and_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("145", "100"),
                    text.replace("150", "1"), text.replace("bg41_01", "bg42_01"),
                    "\n".join(self.rows()[1:]), "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_model_nodes(bad)

    def test_mismatch_cannot_be_hidden_and_input_is_bounded(self):
        text = "\n".join(self.rows())
        for bad in ("[native-model-node-mismatch]\n" + text, text.replace("wrong 0", "wrong 1", 1),
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_model_nodes(bad)
        with self.assertRaises(Pending):
            verify_model_nodes(text + "\n[native-material-context] mode Loading")


class ObjectInputScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-object-inputs] 10 publications 100 owned colour reads 4 unavailable; 100 checks wrong 0; 1 owned primitive packets;"
        rows[4] = "[native-object-inputs] 20 publications 150 owned colour reads 8 unavailable; 150 checks wrong 0; 1 owned primitive packets;"
        return rows

    def test_fresh_complete_comparison_not_a_draw_claim(self):
        self.assertEqual(verify_object_inputs("\n".join(self.rows())),
                         dict(publications_delta=10, reads_delta=50, checks_delta=50, unavailable=8, packets_observed=1))

    def test_stale_reset_missing_checks_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150 owned", "100 owned"), text.replace("150 checks", "149 checks"),
                    text.replace("20 publications", "10 publications"), text.replace("150", "1"),
                    text.replace("bg41_01", "bg42_01"), "\n".join(self.rows()[1:])):
            with self.assertRaises(Pending):
                verify_object_inputs(bad)

    def test_mismatch_and_limits(self):
        text = "\n".join(self.rows())
        for bad in ("[native-object-input-mismatch]\n" + text, text.replace("wrong 0", "wrong 1", 1),
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_object_inputs(bad)
        with self.assertRaises(Pending):
            verify_object_inputs(text + "\n[native-material-context] mode Loading")


class SelectedLightScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-selected-lights] 100 publications 30 changed slots 1 compatibility; 100 checks wrong 0; 100 object snapshots 7 unavailable; 100 draw checks wrong 0;"
        rows[4] = "[native-selected-lights] 150 publications 40 changed slots 2 compatibility; 150 checks wrong 0; 150 object snapshots 9 unavailable; 150 draw checks wrong 0;"
        return rows

    def test_fresh_producer_and_draw_comparison(self):
        self.assertEqual(verify_selected_lights("\n".join(self.rows())), dict(
            publications_delta=50, changed_slots_delta=10, compatibility_delta=1,
            checks_delta=50, snapshots_delta=50, unavailable_delta=2, draw_checks_delta=50))

    def test_stale_reset_incomplete_and_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("150 checks", "149 checks"),
                    text.replace("40 changed", "30 changed"), text.replace("150 object", "100 object"),
                    text.replace("150 draw", "100 draw"), text.replace("2 compatibility", "0 compatibility"),
                    text.replace("bg41_01", "bg42_01"), "\n".join(self.rows()[1:]),
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2]),
                    text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_selected_lights(bad)

    def test_mismatch_and_limits(self):
        text = "\n".join(self.rows())
        for bad in ("[native-selected-light-mismatch]\n" + text, text.replace("checks wrong 0", "checks wrong 1", 1),
                    text.replace("draw checks wrong 0", "draw checks wrong 1", 1), "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_selected_lights(bad)


class LightSelectionScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-light-selection] 100 updates 30 rebuilds 90 candidates 2 compatibility; 100 checks wrong 0;"
        rows[4] = "[native-light-selection] 150 updates 40 rebuilds 120 candidates 2 compatibility; 150 checks wrong 0;"
        return rows

    def test_fresh_scored_selection(self):
        self.assertEqual(verify_light_selection("\n".join(self.rows())), dict(
            updates_delta=50, rebuilds_delta=10, candidates_delta=30, checks_delta=50))

    def test_run941_dirty_mismatch_invalidates_preceding_success(self):
        text = "\n".join(self.rows()) + ("\n[error] [native-light-selection-mismatch] selection 237FA614 view 0 "
                "address 237FA618 actual FFFFFFFF expected FFFFFFFE")
        with self.assertRaisesRegex(ValueError, "native light selection mismatch"):
            verify_light_selection(text)

    def test_stale_missing_scoring_reset_incomplete_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("150 checks", "149 checks"),
                    text.replace("40 rebuilds", "30 rebuilds"), text.replace("120 candidates", "90 candidates"),
                    text.replace("120 candidates", "1 candidates"), text.replace("bg41_01", "bg42_01"),
                    "\n".join(self.rows()[1:]), text + "\n[native-material-context] mode Loading",
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_light_selection(bad)

    def test_mismatch_limits_and_fallback_growth(self):
        text = "\n".join(self.rows())
        for bad in ("[native-light-selection-mismatch]\n" + text, text.replace("wrong 0", "wrong 1", 1),
                    text.replace("120 candidates 2", "120 candidates 3"), "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_light_selection(bad)


class ShadowImageScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-shadow-images] begins 101 ends 100 publications 100; ownership checks 100 wrong 0; compatibility 2 2; empty clears 3;"
        rows[4] = "[native-shadow-images] begins 151 ends 150 publications 150; ownership checks 150 wrong 0; compatibility 2 2; empty clears 4;"
        return rows

    def test_fresh_native_pass_and_exact_handoff(self):
        self.assertEqual(verify_shadow_images("\n".join(self.rows())), dict(
            begins_delta=50, ends_delta=50, publications_delta=50, checks_delta=50, empty_clears_delta=1))

    def test_stale_reset_missing_incomplete_or_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("checks 150", "checks 149"),
                    text.replace("begins 151", "begins 1"), text.replace("empty clears 4", "empty clears 1"),
                    text.replace("bg41_01", "bg42_01"), "\n".join(self.rows()[1:]),
                    text + "\n[native-material-context] mode Loading",
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_shadow_images(bad)

    def test_mismatch_limits_or_fallback_growth(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("wrong 0", "wrong 1", 1), "x" * (MAX_LOG_BYTES + 1),
                    text.replace("compatibility 2 2; empty clears 4", "compatibility 3 2; empty clears 4"),
                    text.replace("compatibility 2 2; empty clears 4", "compatibility 2 3; empty clears 4")):
            with self.assertRaises(ValueError):
                verify_shadow_images(bad)


class PrimitiveShaderScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-primitive-shader] 100 checks wrong 0; 100 owned-input draws;"
        rows[4] = "[native-primitive-shader] 150 checks wrong 0; 160 owned-input draws;"
        return rows

    def test_fresh_comparisons_and_consumption(self):
        self.assertEqual(verify_primitive_shader("\n".join(self.rows())), dict(checks_delta=50, draws_delta=60))

    def test_stale_reset_scene_and_mismatch(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("160", "100"),
                    text.replace("150", "1"), text.replace("bg41_01", "bg42_01"),
                    "\n".join(self.rows()[1:]), text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_primitive_shader(bad)
        for bad in (text.replace("wrong 0", "wrong 1", 1), "[native-primitive-shader-mismatch]\n" + text,
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_primitive_shader(bad)


class LightingPassScenarioTest(unittest.TestCase):
    def test_fresh_pass_comparison_and_consumption(self):
        text = "\n".join(PrimitiveShaderScenarioTest().rows()).replace("native-primitive-shader", "native-lighting-pass")
        self.assertEqual(verify_lighting_pass(text), dict(checks_delta=50, draws_delta=60))
        for bad in (text.replace("150", "100"), text.replace("160", "100"), text.replace("150", "1"),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_lighting_pass(bad)
        for bad in (text.replace("wrong 0", "wrong 1", 1), "[native-lighting-pass-mismatch]\n" + text,
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_lighting_pass(bad)


class MaterialFeatureScenarioTest(unittest.TestCase):
    def test_fresh_features_not_stale_or_unconsumed(self):
        rows = [row.replace("native-primitive-shader", "native-material-feature")
                for row in PrimitiveShaderScenarioTest().rows()]
        text = "\n".join(rows)
        self.assertEqual(verify_material_features(text), dict(checks_delta=50, draws_delta=60))
        for bad in (text.replace("150", "100"), text.replace("160", "100"), text.replace("150", "1"),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading",
                    "\n".join([rows[2], rows[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_material_features(bad)
        for bad in (text.replace("wrong 0", "wrong 1", 1), "[native-material-feature-mismatch]\n" + text,
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_material_features(bad)


class MaterialSamplerScenarioTest(unittest.TestCase):
    def test_fresh_sampler_comparisons_and_consumption(self):
        rows = [row.replace("native-primitive-shader", "native-material-sampler")
                for row in PrimitiveShaderScenarioTest().rows()]
        text = "\n".join(rows)
        self.assertEqual(verify_material_samplers(text), dict(checks_delta=50, draws_delta=60))
        for bad in (text.replace("150", "100"), text.replace("160", "100"), text.replace("150", "1"),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading",
                    "\n".join([rows[2], rows[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_material_samplers(bad)
        for bad in (text.replace("wrong 0", "wrong 1", 1), "[native-material-sampler-mismatch]\n" + text,
                    "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_material_samplers(bad)


class RigidShadowScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-rigid-shadow] frame 100 submitted 100 suppressed 0 fence-retired 99; node 64 instance 144 generation 93 phase 1;"
        rows[4] = "[native-rigid-shadow] frame 150 submitted 150 suppressed 0 fence-retired 149; node 64 instance 144 generation 93 phase 1;"
        return rows

    def test_fresh_native_draws_and_fences(self):
        self.assertEqual(verify_rigid_shadow("\n".join(self.rows())), dict(submitted_delta=50, retired_delta=50))

    def test_stale_missing_generation_change_and_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150 submitted 150", "150 submitted 100"),
                    text.replace("retired 149", "retired 99"),
                    text.replace("bg41_01", "bg42_01"),
                    text + "\n[native-material-context] mode Loading",
                    text.replace("generation 93 phase 1;", "generation 94 phase 1;", 1),
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_rigid_shadow(bad)

    def test_refusal_and_input_bounds(self):
        text = "\n".join(self.rows())
        for bad in ("[native-rigid-shadow] selected node refused: camera\n" + text,
                    text.replace("node 64", "node 63"), "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_rigid_shadow(bad)


class CasterFamilyScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-caster-family] frame 100 nodes 100 multi-primitive nodes 50 submitted 300 emitted 290 fence-retired 280;"
        rows[4] = "[native-caster-family] frame 150 nodes 150 multi-primitive nodes 100 submitted 450 emitted 440 fence-retired 430;"
        return rows

    def test_fresh_non_regression_family(self):
        self.assertEqual(verify_caster_family("\n".join(self.rows())), dict(
            nodes_delta=50, multi_nodes_delta=50, submitted_delta=150, emitted_delta=150, retired_delta=150))

    def test_missing_multi_primitive_stale_reset_and_wrong_scene(self):
        rows = self.rows(); text = "\n".join(rows)
        for bad in (text.replace("nodes 150", "nodes 100"), text.replace("primitive nodes 100", "primitive nodes 50"),
                    text.replace("submitted 450", "submitted 300").replace("emitted 440", "emitted 290").replace("retired 430", "retired 280"),
                    text.replace("frame 150", "frame 100"), text.replace("bg41_01", "bg42_01"),
                    text + "\n[native-material-context] mode Loading",
                    "\n".join([rows[2],rows[4]] + scenario()[::2])):
            with self.assertRaises(Pending): verify_caster_family(bad)

    def test_refusal_invalid_counts_and_bounds(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("emitted 440", "emitted 451"), text.replace("retired 430", "retired 441"),
                    text.replace("primitive nodes 100", "primitive nodes 151"),
                    "[native-rigid-shadow] admitted node refused: owner\n" + text,
                    "[native-caster-family] invalid\n" + text, "x"*(MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError): verify_caster_family(bad)
        with self.assertRaises(ValueError):
            verify_rigid_shadow("[native-rigid-shadow] admitted node refused: owner\n" +
                                "\n".join(RigidShadowScenarioTest().rows()))

    def test_both_reload_epochs_require_the_new_family(self):
        import native_instance_scenario as module
        from unittest.mock import patch
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in vars(module).copy():
                if (name == "verify" or name.startswith("verify_")) and name not in (
                        "verify_rigid_epoch", "verify_caster_family"):
                    stack.enter_context(patch.object(module, name))
            cold, new, _ = split_rigid_reload(RigidReloadScenarioTest.sample())
            for first, second in ((cold, "\n".join(self.rows())), ("\n".join(self.rows()), new)):
                with self.assertRaises(Pending):
                    module.verify_rigid_epoch(first, caster_family=True)
                    module.verify_rigid_epoch(second, caster_family=True)
            module.verify_rigid_epoch("\n".join(self.rows()), caster_family=True)


class CutoutFamilyScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        for index, frame in ((2, 100), (4, 150)):
            counts = (f"submitted {frame+3} emitted {frame+2} fence-retired {frame+1} "
                      f"textured-emitted {frame} textured-retired {frame-1};")
            rows[index] = f"[native-cutout-family] frame {frame} scene {counts} shadow {counts}"
        return rows

    def test_both_consumers_and_textured_retirement_advance(self):
        result = verify_cutout_family("\n".join(self.rows()))
        self.assertEqual(len(result), 10)
        self.assertEqual(set(result.values()), {50})

    def test_opaque_only_stale_untextured_or_wrong_scene_cannot_pass(self):
        rows = self.rows(); text = "\n".join(rows)
        zeros = re.sub(r"(?<!frame )\b\d+\b", "0", rows[2])
        for bad in (text.replace(rows[4], rows[2].replace("frame 100", "frame 150")),
                    text.replace("textured-emitted 150", "textured-emitted 100").replace("textured-retired 149", "textured-retired 99"),
                    text.replace("textured-retired 149", "textured-retired 99"),
                    text.replace(rows[2], zeros).replace(rows[4], zeros.replace("frame 100", "frame 150")),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading",
                    "\n".join([rows[2], rows[4]] + scenario()[::2])):
            with self.assertRaises(Pending): verify_cutout_family(bad)

    def test_refusal_impossible_counts_reset_and_bounds(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("emitted 152", "emitted 154"),
                    text.replace("retired 151", "retired 153"),
                    text.replace("textured-emitted 150", "textured-emitted 153"),
                    text.replace("textured-retired 149", "textured-retired 151"),
                    text.replace("frame 150", "frame 99"),
                    text.replace("textured-retired 149", "textured-retired 98"),
                    "[native-rigid-shadow] admitted node refused: owner\n" + text,
                    "[native-rigid-scene] node refused: owner\n" + text,
                    "[native-cutout-family] invalid\n" + text, "x"*(MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError): verify_cutout_family(bad)

    def test_each_reload_epoch_requires_both_consumers(self):
        import native_instance_scenario as module
        from unittest.mock import patch
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in vars(module).copy():
                if (name == "verify" or name.startswith("verify_")) and name not in (
                        "verify_rigid_epoch", "verify_cutout_family"):
                    stack.enter_context(patch.object(module, name))
            good = "\n".join(self.rows())
            missing = "\n".join(scenario())
            for first, second in ((missing, good), (good, missing)):
                with self.assertRaises(Pending):
                    module.verify_rigid_epoch(first, cutout_family=True)
                    module.verify_rigid_epoch(second, cutout_family=True)
            module.verify_rigid_epoch(good, cutout_family=True)
            module.verify_rigid_epoch(good, cutout_family=True)


class RigidSceneScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-rigid-scene] frame 100 submitted 100 emitted 99 suppressed 0 fence-retired 98; node 64 instance 144 generation 93;"
        rows[4] = "[native-rigid-scene] frame 150 submitted 150 emitted 149 suppressed 0 fence-retired 148; node 64 instance 144 generation 93;"
        return rows

    def test_emitted_commands_and_fences(self):
        self.assertEqual(verify_rigid_scene("\n".join(self.rows())),
                         dict(submitted_delta=50, emitted_delta=50, retired_delta=50))

    def test_queue_only_stale_reset_or_wrong_scene_cannot_pass(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("emitted 149", "emitted 99").replace("retired 148", "retired 98"),
                    text.replace("emitted 149", "emitted 148"), text.replace("retired 148", "retired 98"),
                    text.replace("generation 93;", "generation 94;", 1),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading",
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2])):
            with self.assertRaises(Pending):
                verify_rigid_scene(bad)

    def test_refusal_impossible_counts_and_bounds(self):
        text = "\n".join(self.rows())
        for bad in ("[native-rigid-scene] selected node refused: receiver\n" + text,
                    text.replace("emitted 149", "emitted 151"), text.replace("retired 148", "retired 150"),
                    text.replace("node 64", "node 63"), "x" * (MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError):
                verify_rigid_scene(bad)


class RigidBatchScenarioTest(unittest.TestCase):
    def text(self):
        rows = scenario()
        rows[2] = "[native-rigid-batch] frame 100 scene instances 100 indirect calls 100 shadow instances 100 indirect calls 100 merged instances 0;"
        rows[4] = "[native-rigid-batch] frame 150 scene instances 150 indirect calls 150 shadow instances 150 indirect calls 150 merged instances 0;"
        return "\n".join(rows)

    def test_fresh_indirect_instances_do_not_claim_merged_runtime_groups(self):
        result = verify_rigid_batches(self.text())
        self.assertEqual(result,dict(scene_instances_delta=50,scene_calls_delta=50,
                                    shadow_instances_delta=50,shadow_calls_delta=50,merged_instances_delta=0))

    def test_each_consumer_must_advance_in_the_ready_scene(self):
        text = self.text()
        for bad in (text.replace("instances 150","instances 100").replace("calls 150","calls 100"),
                    text.replace("calls 150","calls 100"), text.replace("frame 150","frame 100"),
                    text.replace("bg41_01","bg42_01"), text+"\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_rigid_batches(bad)

    def test_refusals_impossible_counts_and_size_are_errors(self):
        text = self.text()
        for bad in ("[native-rigid-batch] refused storage\n"+text,
                    text.replace("calls 150","calls 151"),text.replace("merged instances 0","merged instances 9999"),
                    "x"*(MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError):
                verify_rigid_batches(bad)


class RigidHardOffScenarioTest(unittest.TestCase):
    def text(self):
        rows = scenario()
        rows[2] = "[native-rigid-hard-off] frame 100 scene checks 100 shadow checks 100; node 64 generation 93;"
        rows[4] = "[native-rigid-hard-off] frame 150 scene checks 150 shadow checks 150; node 64 generation 93;"
        return "\n".join([
            "[native-rigid-hard-off] frame 1 scene checks 0 shadow checks 1; node 64 generation 93;",
            "[native-rigid-shadow] frame 1 submitted 1"] + rows)

    def test_fresh_admission_not_reload_or_emission_proof(self):
        self.assertEqual(verify_rigid_hard_off(self.text()),
                         dict(scene_checks_delta=50, shadow_checks_delta=50))

    def test_late_enable_missing_consumer_and_mixed_generations_cannot_pass(self):
        text = self.text()
        for bad in ("\n".join(text.splitlines()[1:]),
                    text.replace("shadow checks 150", "shadow checks 100"),
                    text.replace("scene checks 150", "scene checks 100"),
                    text.replace("frame 150", "frame 100"),
                    text.replace("frame 150 scene checks 150 shadow checks 150; node 64 generation 93;",
                                 "frame 150 scene checks 150 shadow checks 150; node 64 generation 94;"),
                    text.replace("bg41_01", "bg42_01"),
                    text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_rigid_hard_off(bad)

    def test_refusal_and_invalid_generation_remain_visible(self):
        for bad in ("[native-rigid-hard-off] refused: selected pose\n" + self.text(),
                    self.text().replace("generation 93", "generation 0"),
                    "x" * (MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError):
                verify_rigid_hard_off(bad)


class ReceiverSetupScenarioTest(unittest.TestCase):
    def text(self):
        rows = scenario()
        for index, frame in ((2, 100), (4, 150)):
            rows[index] = (f"[native-shadow-receiver] frame {frame} native {frame} ignored 0 original 0 refused 0; "
                           f"compatibility bindings {frame} parameters {frame}; owned packets {frame} reads {frame} missing 0;")
        return "\n".join(rows)

    def test_fresh_native_callback_and_owned_reads(self):
        self.assertEqual(verify_receiver_setup(self.text()),
                         dict(native_delta=50, packets_delta=50, reads_delta=50, original=0))

    def test_publication_alone_stale_wrong_scene_and_empty_cannot_pass(self):
        text = self.text()
        for bad in ("", text.replace("reads 150", "reads 100"),
                    text.replace("frame 150", "frame 100"),
                    text.replace("bg41_01", "bg42_01"),
                    text + "\n[native-material-context] mode Loading",
                    text.replace("native 150", "native 100")):
            with self.assertRaises(Pending): verify_receiver_setup(bad)

    def test_original_refusal_missing_and_impossible_counts_are_errors(self):
        text = self.text()
        for bad in ("[native-shadow-receiver] refused: descriptor\n" + text,
                    text.replace("original 0", "original 1"), text.replace("refused 0", "refused 1"),
                    text.replace("missing 0", "missing 1"), text.replace("ignored 0", "ignored 999"),
                    text.replace("packets 150", "packets 151"), "x"*(MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError): verify_receiver_setup(bad)

    def test_reload_requires_receiver_check_in_each_epoch(self):
        # Exercise the real epoch orchestrator; other independently tested
        # consumer gates are stubbed so this fixture isolates new gate wiring.
        import native_instance_scenario as module
        from unittest.mock import patch
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in vars(module).copy():
                if (name == "verify" or name.startswith("verify_")) and name not in (
                        "verify_rigid_epoch", "verify_receiver_setup"):
                    stack.enter_context(patch.object(module, name))
            cold, new, _ = split_rigid_reload(RigidReloadScenarioTest.sample())
            for old_text, new_text in ((cold, self.text()), (self.text(), new)):
                with self.assertRaises(Pending):
                    module.verify_rigid_epoch(old_text, receiver_setup=True)
                    module.verify_rigid_epoch(new_text, receiver_setup=True)
            module.verify_rigid_epoch(self.text(), receiver_setup=True)


class SceneLightsScenarioTest(unittest.TestCase):
    def text(self):
        rows = scenario()
        for index, frame in ((2, 100), (4, 150)):
            rows[index] = (f"[native-scene-lights] frame {frame} update {frame} pass update {frame} light view 0; "
                           f"{frame} publications 0 refused; 500 bindings 2 unavailable imports; "
                           f"{frame} native reads 0 missing; instance 11 generation 93 node 64;")
        return "\n".join(rows)

    def test_owned_scene_lights_have_fresh_handoffs_and_consumers(self):
        self.assertEqual(verify_scene_lights(self.text()), dict(updates_delta=50, reads_delta=50,
                         bindings=500, unavailable_imports=2, generation=93, light_view=0))

    def test_stale_publication_wrong_scene_and_missing_consumption_cannot_pass(self):
        text = self.text()
        for bad in ("", text.replace("150 native reads", "100 native reads"),
                    text.replace("150 publications", "100 publications"), text.replace("frame 150", "frame 100"),
                    text.replace("bg41_01", "bg42_01"), text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending): verify_scene_lights(bad)

    def test_stale_pass_fault_and_bounds_are_errors(self):
        text = self.text()
        for bad in (text.replace("pass update 150", "pass update 100"), text.replace("0 missing", "1 missing"),
                    text.replace("light view 0", "light view 16"), text.replace("500 bindings", "65537 bindings"),
                    text.replace("instance 11", "instance 0"), text.replace("150 publications 0 refused", "150 publications 1 refused"),
                    "[native-scene-lights] handoff failed: source\n" + text, "x"*(MAX_LOG_BYTES+1)):
            with self.assertRaises(ValueError): verify_scene_lights(bad)

    def test_reload_checks_each_epoch_not_only_the_final_field(self):
        import native_instance_scenario as module
        from unittest.mock import patch
        from contextlib import ExitStack
        with ExitStack() as stack:
            for name in vars(module).copy():
                if (name == "verify" or name.startswith("verify_")) and name not in (
                        "verify_rigid_epoch", "verify_scene_lights"):
                    stack.enter_context(patch.object(module, name))
            cold, new, _ = split_rigid_reload(RigidReloadScenarioTest.sample())
            for old_text, new_text in ((cold, self.text()), (self.text(), new)):
                with self.assertRaises(Pending):
                    module.verify_rigid_epoch(old_text, scene_lights=True)
                    module.verify_rigid_epoch(new_text, scene_lights=True)
            module.verify_rigid_epoch(self.text(), scene_lights=True)


class FogScenarioTest(unittest.TestCase):
    def rows(self):
        rows = scenario()
        rows[2] = "[native-fog] 100 updates 10 inactive 1 compatibility 2 resets; 110 checks wrong 0; 100 object snapshots 7 unavailable; 100 draw checks 100 active layers wrong 0;"
        rows[4] = "[native-fog] 150 updates 20 inactive 2 compatibility 2 resets; 170 checks wrong 0; 150 object snapshots 9 unavailable; 150 draw checks 150 active layers wrong 0;"
        return rows

    def test_fresh_active_fog_and_complete_publication_checks(self):
        self.assertEqual(verify_fog("\n".join(self.rows())), dict(
            updates_delta=50, inactive_delta=10, compatibility_delta=1, resets_delta=0,
            checks_delta=60, snapshots_delta=50, unavailable_delta=2, draw_checks_delta=50, active_layers_delta=50))

    def test_stale_reset_incomplete_inactive_and_wrong_scene(self):
        text = "\n".join(self.rows())
        for bad in (text.replace("150", "100"), text.replace("170 checks", "169 checks"),
                    text.replace("150 object", "100 object"), text.replace("150 draw", "100 draw"),
                    text.replace("150 active", "100 active"), text.replace("2 compatibility", "0 compatibility"),
                    text.replace("bg41_01", "bg42_01"), "\n".join(self.rows()[1:]),
                    "\n".join([self.rows()[2], self.rows()[4]] + scenario()[::2]),
                    text + "\n[native-material-context] mode Loading"):
            with self.assertRaises(Pending):
                verify_fog(bad)

    def test_mismatch_and_limits(self):
        text = "\n".join(self.rows())
        for bad in ("[native-fog-mismatch]\n" + text, text.replace("checks wrong 0", "checks wrong 1", 1),
                    text.replace("layers wrong 0", "layers wrong 1", 1), "x" * (MAX_LOG_BYTES + 1)):
            with self.assertRaises(ValueError):
                verify_fog(bad)


class RigidReloadScenarioTest(unittest.TestCase):
    @staticmethod
    def sample():
        def row(event,gen,instance,retired,counts):
            return f"[native-rigid-lifecycle] {event} generation {gen} instance {instance} source-retired {retired} scene {counts} shadow {counts};"
        return "\n".join((
            row("loaded",93,0,0,"0/0/0"), "cold field evidence",
            row("cold-qualified",93,144,0,"1002/1000/998"),
            "[native-rigid-reload] window generation 93 scene 100->1000 shadow 100->1000",
            "[native-rigid-reload] title requested generation 93 instance 144 task-uid 24 sequence-id 2",
            row("source-retired",93,144,1,"1004/1002/1000"),
            row("closed-at-title",93,144,1,"1004/1004/1004"),
            "[native-rigid-reload] title reached; old generation 93 fully fence-retired; autoplay epoch 1",
            row("loaded",193,0,0,"0/0/0"), "reloaded field evidence",
            row("reload-qualified",193,288,0,"1102/1100/1098"),
            "[native-rigid-reload] window generation 193 scene 200->1100 shadow 200->1100",
            "[native-rigid-reload] complete old-generation 93 new-generation 193 old-instance 144 new-instance 288;"))

    def test_split_keeps_epochs_independent(self):
        cold,new,proof = split_rigid_reload(self.sample())
        self.assertIn("cold field evidence",cold)
        self.assertNotIn("reloaded field evidence",cold)
        self.assertIn("reloaded field evidence",new)
        self.assertNotIn("cold field evidence",new)
        self.assertEqual(proof["new_generation"],193)

    def test_pending_and_limits(self):
        with self.assertRaises(Pending):
            split_rigid_reload(self.sample().split("[native-rigid-reload] complete")[0])
        for bad in ("x"*(2*MAX_LOG_BYTES+1),self.sample()+"\n[error] fault",
                    "x"*MAX_LOG_BYTES+self.sample(), self.sample()+self.sample()):
            with self.assertRaises(ValueError): split_rigid_reload(bad)

    def test_requires_real_fresh_lifetimes_and_both_output_windows(self):
        text = self.sample()
        for bad in (text.replace("new-generation 193","new-generation 93"),
                    text.replace("new-instance 288","new-instance 144"),
                    text.replace("source-retired generation","aggregate-retired generation"),
                    text.replace("1004/1004/1004","1004/1004/1003"),
                    text.replace("1004/1002/1000","999/998/997"),
                    text.replace("200->1100 shadow 200->1100","201->1100 shadow 200->1100"),
                    text.replace("epoch 1","epoch 2"),
                    text.replace("task-uid 24","task-uid 0"),
                    text.splitlines()[-1]+"\n"+"\n".join(text.splitlines()[:-1]),
                    text.replace("loaded generation 193 instance 0","loaded generation 193 instance 288")):
            with self.subTest(bad=bad):
                with self.assertRaises(ValueError): split_rigid_reload(bad)

    def test_939_guard_before_context_needs_another_window_not_weaker_freshness(self):
        def guard(frame,count):
            return f"[native-rigid-hard-off] frame {frame} scene checks {count} shadow checks {count+1}; node 64 generation 93;"
        rows = [guard(745,0), "[native-rigid-shadow] frame 745 submitted 1",
                "[native-material-context] frame 1345 " + READY.replace("event 0", "event 1"),
                guard(1645,900), "[native-material-context] frame 1645 " + READY,
                guard(1945,1200), "[native-material-context] frame 1945 " + READY]
        with self.assertRaises(Pending): verify_rigid_hard_off("\n".join(rows))
        rows += [guard(2245,1500), "[native-material-context] frame 2245 " + READY]
        self.assertEqual(verify_rigid_hard_off("\n".join(rows))["scene_checks_delta"],300)

    def test_942_title_exit_is_terminal_not_a_pending_field(self):
        shutdown = "[warning] [shutdown] requested (guest-exit)\n[shutdown] complete, exiting 0"
        for text in (shutdown, self.sample()+"\n"+shutdown):
            with self.assertRaisesRegex(ValueError, "title-menu Exit"):
                split_rigid_reload(text)


if __name__ == "__main__":
    unittest.main()
