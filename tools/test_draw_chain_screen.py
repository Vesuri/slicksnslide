import struct
import unittest
from draw_chain_screen import decode, dispatch_counts
from test_order_reuse_screen import record


def capture(groups, point_handles):
    indices = [-1] * 200
    for h in point_handles:
        indices[h] = h
    return record(98, groups, stale=199) + struct.pack(">200h", *indices)


class DrawChainTest(unittest.TestCase):
    def test_alternating(self):
        row, = decode(capture({0: [1, 2, 3, 4, 5]}, [2, 4]))
        self.assertEqual((row['runs'], row['transitions'], row['points']), (5, 4, 2))

    def test_contiguous_and_priority_boundaries(self):
        row, = decode(capture({0: [1, 2, 3, 4], 127: [199]}, [3, 4, 199]))
        self.assertEqual((row['runs'], row['transitions'], row['occupied']), (3, 1, 2))

    def test_empty_and_stale(self):
        row, = decode(capture({}, [199]))
        self.assertEqual(row['runs'], 0)
        self.assertEqual(row['handles'], 0)

    def test_invalid(self):
        base = capture({0: [1]}, [])
        bad = bytearray(base)
        struct.pack_into('>h', bad, len(base)-400+2, 256)
        for data in (b'', base[:-1], bad, base+base):
            with self.assertRaises(ValueError):
                decode(data)

    def test_dispatch_windows(self):
        rows = decode(capture({0: [1, 2]}, [2]))
        text = 'DRAW_DISPATCH frame=98 point=1 sprite=2 general=1\n'
        self.assertEqual(dispatch_counts(text, rows)[98]['sprite_calls'], 2)
        for broken in ('', text+text, text.replace('98', '99')):
            with self.assertRaises(ValueError):
                dispatch_counts(broken, rows)


if __name__ == '__main__':
    unittest.main()
