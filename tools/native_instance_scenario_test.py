import unittest
from native_instance_scenario import MAX_LOG_BYTES, Pending, READY, verify


def metric(reads=100, checks=100, wrong=0, refused=0):
    return (f"[native-instances] 12 created 1 retired 11 live / 9000 bytes; "
            f"20 poses published 50 reused; 70 producer imports {refused} refused; "
            f"{reads} consumer reads 3 unavailable; {checks} checks wrong {wrong};")


def scenario():
    return ["[native-material-context] stage bg41_01 player 1 event 1 movie 0",
            "[native-material-context] " + READY, metric(),
            "[native-material-context] " + READY, metric(150, 140)]


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


if __name__ == "__main__":
    unittest.main()
