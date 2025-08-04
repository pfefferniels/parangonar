# Parangonar

**Parangonar** is a Python package for symbolic music note alignment. 
This package contains the AutomaticNoteMatcher algorithm for aligning musical score and performance data.

## Installation

The easiest way to install the package is via `pip` from the [PyPI (Python
Package Index)](https://pypi.org/project/parangonar/):
```shell
pip install parangonar
```
This will install the latest release of the package and will install all dependencies automatically.

## Getting Started

**Parangonar** contains the AutomaticNoteMatcher algorithm:

- `AutomaticNoteMatcher`: 
    Piano roll-based, hierarchical DTW and combinatorial optimization for pitch-wise note distribution.
    Requires scores and performances in the current implementation.

## Usage

```python
from parangonar import AutomaticNoteMatcher, fscore_alignments
import partitura as pt

# Load your score and performance
score = pt.load_score('your_score.musicxml')
performance = pt.load_performance('your_performance.mid')

# Create note arrays
score_note_array = score.note_array()
performance_note_array = performance.note_array()

# Create and use the matcher
matcher = AutomaticNoteMatcher()
alignment = matcher(score_note_array, performance_note_array)

# Evaluate if you have ground truth
# f_score = fscore_alignments(alignment, ground_truth, "match")
```

## License

The code in this package is licensed under the Apache 2.0 License. For details,
please see the [LICENSE](LICENSE) file.