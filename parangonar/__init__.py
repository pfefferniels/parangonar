#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
The top level of the package contains functions to
note-align symbolic music data.
"""

from .match import AutomaticNoteMatcher
from .evaluate import fscore_alignments

__all__ = [
    "AutomaticNoteMatcher",
    "fscore_alignments",
]
