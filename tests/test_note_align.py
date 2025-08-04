#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
This module includes tests for alignment utilities.
"""
import unittest
import numpy as np
from parangonar import (
    AutomaticNoteMatcher,
    fscore_alignments,
)
import partitura as pt

RNG = np.random.RandomState(1984)
from tests import MATCH_FILES


class TestNoteAlignment(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.perf_match, cls.alignment, cls.score_match = pt.load_match(
            filename=MATCH_FILES[0], create_score=True
        )

    def test_AutomaticNoteMatcher_align(self, **kwargs):
        pna_match = self.perf_match.note_array()
        sna_match = self.score_match.note_array()
        matcher = AutomaticNoteMatcher()
        pred_alignment = matcher(sna_match, pna_match)
        _, _, f_score = fscore_alignments(pred_alignment, self.alignment, "match")
        self.assertTrue(f_score == 1.0)


if __name__ == "__main__":
    unittest.main()
