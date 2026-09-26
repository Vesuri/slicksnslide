"""Schema/bounds regressions; no original game bytes are used here."""
import csv
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

TOOL = Path(__file__).with_name("summarize_primitive_calls.py")
FIELDS = ["target", "caller", "name", *(f"arg{i}" for i in range(9)),
          "stride", "source_width", "source_height", "source_hash", "count"]


def call(target, name, arguments):
    row = dict.fromkeys(FIELDS, "0")
    row.update(target=hex(target), caller="0", name=name, stride="100",
               source_width="80", source_height="200", count="1")
    for i in range(9):
        row[f"arg{i}"] = str(arguments[i]) if i < len(arguments) else ""
    return row


class PrimitiveSummaryTests(unittest.TestCase):
    def audit(self, row, partial=True):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "calls.csv"
            with path.open("w", newline="") as stream:
                writer = csv.DictWriter(stream, fieldnames=FIELDS)
                writer.writeheader()
                writer.writerow(row)
            return subprocess.run([sys.executable, "-B", str(TOOL), str(path),
                                   *(["--partial"] if partial else [])],
                                  capture_output=True, text=True)

    def test_surface_legacy_and_named(self):
        for name in ("unknown", "surface_event"):
            result = self.audit(call(0xE6B1, name, [1, 2, 3, 0, 1]))
            self.assertEqual(result.returncode, 0, result.stderr)

    def test_unknown_target_still_fails(self):
        self.assertNotEqual(self.audit(call(0x12345, "unknown", [])).returncode, 0)

    def test_wrong_surface_arity_still_fails(self):
        self.assertNotEqual(self.audit(call(0xE6B1, "unknown", [0]*6)).returncode, 0)

    def test_height_is_a_byte(self):
        result = self.audit(call(0x2B8DE, "vga_planar_subrect_blit",
                                 [0, 0, 0, 190, 16, 0x3408, 4, 0, 0]))
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("extends past", result.stdout)

    def test_real_source_overrun_is_reported(self):
        result = self.audit(call(0x2B8DE, "vga_planar_subrect_blit",
                                 [0, 0, 35, 105, 100, 150, 4, 0, 0]))
        self.assertIn("extends past", result.stdout)

    def test_full_sequence_required_by_default(self):
        self.assertNotEqual(self.audit(call(0xE6B1, "unknown", [0]*5),
                                       partial=False).returncode, 0)


if __name__ == "__main__":
    unittest.main()
