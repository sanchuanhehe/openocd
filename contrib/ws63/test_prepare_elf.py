#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0-only
"""Offline regressions for synthesized unwind information (standard library only)."""

import unittest

from prepare_elf import cfi_rows, unwind_states


class StackStores(unittest.TestCase):
    def analyze(self, body):
        instructions = {
            0x100 + 4 * i: (4, op, args) for i, (op, args) in enumerate(body)
        }
        size = 4 * len(body)
        states = unwind_states(0x100, size, instructions)
        cfi = cfi_rows(0x100, size, states) if states else None
        return states, cfi

    def test_reused_old_copy_uses_surviving_return_address(self):
        states, cfi = self.analyze([
            ("addi", "sp,sp,-16"),
            ("sw", "ra,12(sp)"),
            ("sw", "ra,8(sp)"),
            ("sw", "a0,12(sp)"),
            ("jal", "ra,200 <callee>"),
            ("lw", "ra,8(sp)"),
            ("addi", "sp,sp,16"),
            ("ret", ""),
        ])
        self.assertIsNotNone(cfi)
        self.assertEqual(states[0x114][1][1], -8)
        self.assertNotIn(1, states[0x118][1])
        self.assertIn(1, states[0x118][3])

    def test_overlapping_store_rejects_unreliable_return_address(self):
        for op, args in [
            ("sb", "a0,15(sp)"), ("sh", "a0,14(sp)"),
            ("sw", "a0,12(sp)"), ("fsw", "fa0,12(sp)"),
            ("fsd", "fa0,8(sp)"), ("sd", "a0,8(sp)"),
            ("stmia", "{a0-a1},(sp)"),
        ]:
            with self.subTest(op=op):
                slot = "4(sp)" if op == "stmia" else "12(sp)"
                _, cfi = self.analyze([
                    ("addi", "sp,sp,-16"), ("sw", "ra," + slot),
                    (op, args), ("jal", "ra,200 <callee>"), ("ret", ""),
                ])
                self.assertIsNone(cfi)

    def test_adjacent_store_preserves_saved_word(self):
        for op, args in [("sb", "a0,11(sp)"), ("sh", "a0,10(sp)"), ("sw", "a0,8(sp)")]:
            with self.subTest(op=op):
                states, cfi = self.analyze([
                    ("addi", "sp,sp,-16"), ("sw", "ra,12(sp)"),
                    (op, args), ("jal", "ra,200 <callee>"),
                    ("lw", "ra,12(sp)"), ("addi", "sp,sp,16"), ("ret", ""),
                ])
                self.assertIsNotNone(cfi)
                self.assertEqual(states[0x110][1][1], -4)

    def test_store_through_frame_pointer_invalidates_slot(self):
        _, cfi = self.analyze([
            ("addi", "sp,sp,-16"), ("sw", "ra,12(sp)"), ("mv", "t0,sp"),
            ("sh", "a0,14(t0)"), ("jal", "ra,200 <callee>"), ("ret", ""),
        ])
        self.assertIsNone(cfi)

    def test_push_pop_still_unwinds(self):
        states, cfi = self.analyze([
            ("push", "{ra,s0-s1},-16"), ("jal", "ra,200 <callee>"),
            ("pop", "{ra,s0-s1},16"), ("ret", ""),
        ])
        self.assertIsNotNone(cfi)
        self.assertEqual(states[0x108][1], {1: -4, 8: -8, 9: -12})
        self.assertEqual(states[0x10C][1], {})

    def test_push_overwrites_a_prior_slot(self):
        _, cfi = self.analyze([
            ("sw", "ra,-4(sp)"), ("push", "{s0},-16"),
            ("jal", "ra,200 <callee>"), ("ret", ""),
        ])
        self.assertIsNone(cfi)

    def test_stmia_ldmia_still_unwinds(self):
        states, cfi = self.analyze([
            ("addi", "sp,sp,-16"), ("stmia", "{ra,s0-s1},(sp)"),
            ("jal", "ra,200 <callee>"), ("ldmia", "{ra,s0-s1},(sp)"),
            ("addi", "sp,sp,16"), ("ret", ""),
        ])
        self.assertIsNotNone(cfi)
        self.assertEqual(states[0x10C][1], {1: -8, 8: -12, 9: -16})
        self.assertEqual(states[0x110][1], {})

    def test_branch_with_overwritten_slot_is_rejected(self):
        _, cfi = self.analyze([
            ("addi", "sp,sp,-16"), ("sw", "ra,12(sp)"),
            ("beq", "a0,zero,110 <join>"), ("sw", "a0,12(sp)"),
            ("jal", "ra,200 <callee>"), ("ret", ""),
        ])
        self.assertIsNone(cfi)

    def test_unknown_stack_adjustment_is_rejected(self):
        states, _ = self.analyze([("sub", "sp,sp,a0"), ("ret", "")])
        self.assertEqual(states, {})


if __name__ == "__main__":
    unittest.main(verbosity=2)
