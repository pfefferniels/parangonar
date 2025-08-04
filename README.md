# Parangonar C++ Port

This is a C++ port of the Python-based library [parangonar](https://github.com/sildater/parangonar)
for symbolic music note alignment. It's specific purpose is to be compiled with emscripten to WebAssembly, 
so that it can be used from within the browser.

This port only implements the AutomaticNoteMatcher algorithm for aligning musical score and performance data.
The port was created for the most part using Gihub's Copilot Agent Mode.

## Building

### Prerequisites

- CMake 3.14 or later
- C++17 compatible compiler (GCC 7+, Clang 5+, MSVC 2017+)
- Optional: Eigen3 for optimized linear algebra (uses internal implementations if not found)

### Standard Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run Tests

```bash
# In build directory
./test_parangonar_cpp

# Or with CTest
ctest -V
```

### Emscripten Build

```bash
# Install emscripten first
# https://emscripten.org/docs/getting_started/downloads.html
# Or: sudo apt install emscripten

mkdir build_emscripten
cd build_emscripten
emcmake cmake ..
emmake make parangonar_wasm
```

This generates:
- `parangonar.js` - WASM module JavaScript interface
- `parangonar.wasm` - WebAssembly binary
- `parangonar-wrapper.js` - High-level JavaScript/TypeScript wrapper

See `build_emscripten/README.md` for detailed WASM usage instructions.

## Usage

### C++ Library

```cpp
#include <parangonar/matchers.hpp>
#include <parangonar/note.hpp>

using namespace parangonar;

// Create note arrays (score and performance)
NoteArray score_notes = ...;
NoteArray performance_notes = ...;

// Create matcher with default configuration
AutomaticNoteMatcher matcher;

// Or with custom configuration
AutomaticNoteMatcherConfig config;
config.sfuzziness = 2.0f;
config.alignment_type = "dtw";
AutomaticNoteMatcher custom_matcher(config);

// Perform alignment
auto alignment = matcher(score_notes, performance_notes);

// Evaluate results (if you have ground truth)
auto ground_truth = ...;
auto fscore_result = evaluation::fscore_matches(alignment, ground_truth);
std::cout << "F-score: " << fscore_result.f_score << std::endl;
```

### WebAssembly (JavaScript/TypeScript)

```javascript
// Simple browser usage
const Module = {
    onRuntimeInitialized: function() {
        // Create notes
        const scoreNotes = new Module.NoteArray();
        scoreNotes.push_back(Module.createScoreNote(0.0, 0.5, 60, "C4"));
        
        const perfNotes = new Module.NoteArray();
        perfNotes.push_back(Module.createPerformanceNote(0.1, 0.4, 60, 70, "p1"));
        
        // Align notes
        const alignment = Module.match(scoreNotes, perfNotes);
        
        // Process results...
    }
};
```

Or using the high-level wrapper:

```javascript
import { createAligner } from './parangonar-wrapper.js';

const aligner = createAligner(() => import('./parangonar.js'));
await aligner.ready();

const alignment = aligner.align(scoreNotes, performanceNotes, {
    sfuzziness: 4.0,
    pfuzziness: 4.0
});
```

See `build_emscripten/README.md` for complete WASM documentation.

## Algorithms

The C++ implementation includes:

- **AutomaticNoteMatcher**: Piano roll-based, hierarchical DTW and combinatorial optimization for pitch-wise note distribution
- **Dynamic Time Warping**: Multiple DTW implementations with custom step patterns and weights
- **Evaluation**: F-score based evaluation supporting match, insertion, and deletion evaluation

For detailed documentation, see [cpp/README.md](cpp/README.md).

## License

The code in this package is licensed under the Apache 2.0 License. For details,
please see the [LICENSE](LICENSE) file.
