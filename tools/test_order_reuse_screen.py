#!/usr/bin/env python3
"""Independent synthetic coverage for the order-reuse screening parser."""
import struct
import unittest

from order_reuse_screen import decode, summarize, RECORD_SIZE


def record(frame, groups, stale=0):
    heads, tails = bytearray(128), bytearray(128)
    following = bytearray([stale] * 200)
    previous = bytearray([stale] * 200)
    for priority, handles in groups.items():
        heads[priority], tails[priority] = handles[0], handles[-1]
        for index, handle in enumerate(handles):
            following[handle] = handles[index + 1] if index + 1 < len(handles) else 0
            previous[handle] = handles[index - 1] if index else 0
    return (struct.pack(">I", frame) + heads + following +
            bytes([max(groups, default=0)]) + tails + previous)


class OrderScreenTest(unittest.TestCase):
    def test_stale_bytes_do_not_prevent_reuse(self):
        data = record(98, {0: [1, 2], 127: [199]}, 45)
        data += record(99, {0: [1, 2], 127: [199]}, 81)
        self.assertEqual(len(data), 2 * RECORD_SIZE)
        self.assertEqual(summarize(decode(data))["unchanged"], 1)

    def test_membership_and_priority_changes(self):
        data = b"".join(record(98 + i, groups) for i, groups in enumerate([
            {0: [1, 2]}, {0: [1]}, {0: [1], 1: [2]}, {0: [1, 2]},
            {0: [1, 2]}, {}, {}, {127: [199]}]))
        result = summarize(decode(data))
        self.assertEqual(result["transitions"], 7)
        self.assertEqual(result["unchanged"], 2)
        self.assertEqual(result["reusable_linked_handles"], 2)
        self.assertEqual(result["changed_handles_mean"], 6 / 7)
        self.assertEqual(result["changed_handles_max"], 2)

    def test_complete_window(self):
        data = b"".join(record(i, {3: list(range(1, 200))}) for i in range(98, 701))
        result = summarize(decode(data))
        self.assertEqual(result["busy_transitions"], 602)
        self.assertEqual(result["busy_unchanged"], 602)
        self.assertEqual(result["reusable_handle_percent"], 100)

    def test_corruption_rejected(self):
        base = record(98, {0: [1, 2], 127: [199]})
        # Invalid head, cycle, duplicate membership, bad inverse, wrong max.
        for offset, value in [(4, 200), (134, 1), (5, 1), (461 + 2, 0), (332, 0)]:
            with self.subTest(offset=offset):
                changed = bytearray(base)
                changed[offset] = value
                with self.assertRaises(ValueError):
                    decode(changed)
        for data in [b"", base[:-1], base + base, base + record(100, {})]:
            with self.assertRaises(ValueError):
                decode(data)

    def test_unstable_order_rejected(self):
        with self.assertRaises(ValueError):
            decode(record(98, {0: [2, 1]}))


if __name__ == "__main__":
    unittest.main()
