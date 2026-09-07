import unittest
import re
from native_instance_scenario import (
    MAX_LOG_BYTES, Pending, READY, verify, verify_texture_tables,
    verify_vertex_inputs, verify_movement, verify_canonical_geometry,
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


if __name__ == "__main__":
    unittest.main()
