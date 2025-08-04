# Parangonar C++ Implementation

This is a C++ port of the Parangonar symbolic music note alignment library, derived using GitHub Copilot from the original Python parangonar project. The C++ implementation focuses on the core AutomaticNoteMatcher algorithm and is designed to be compilable with emscripten for web deployment.

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

mkdir build_emscripten
cd build_emscripten
emcmake cmake ..
emmake make
```

## Usage

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

## Data Structures

### Note

Represents a musical note with timing and pitch information:

```cpp
struct Note {
    // Score fields
    float onset_beat, duration_beat;
    // Performance fields  
    float onset_sec, duration_sec;
    int velocity;
    // Common fields
    int pitch;
    std::string id;
    // ... other fields
};
```

### NoteArray

Collection of notes with utility methods:

```cpp
class NoteArray {
    std::vector<Note> notes;
    
    // Filter by pitch
    NoteArray filter_by_pitch(int pitch) const;
    
    // Get unique pitches
    std::vector<int> unique_pitches() const;
    
    // Compute piano roll representation
    std::vector<std::vector<float>> compute_pianoroll(int time_div = 16) const;
};
```

### Alignment

Represents the alignment between score and performance notes:

```cpp
struct Alignment {
    enum class Label { MATCH, INSERTION, DELETION };
    Label label;
    std::string score_id;
    std::string performance_id;
};
```

## Algorithms

### AutomaticNoteMatcher

The main algorithm equivalent to Python's `PianoRollNoNodeMatcher`:

1. **Initial DTW**: Coarse time warping to generate anchor points
2. **Cutting**: Split note arrays into windows based on alignment times  
3. **Fine Alignment**: Detailed DTW and symbolic matching within windows
4. **Mending**: Combine windowed alignments into global alignment

### Dynamic Time Warping

Multiple DTW implementations:

- `DynamicTimeWarping`: Standard DTW
- `WeightedDynamicTimeWarping`: DTW with custom step patterns and weights

### Evaluation

F-score based evaluation supporting:
- Match evaluation
- Insertion/deletion evaluation  
- Combined evaluation across multiple alignment types

## Configuration

The `AutomaticNoteMatcherConfig` structure provides extensive customization:

```cpp
struct AutomaticNoteMatcherConfig {
    std::string alignment_type = "dtw";          // "dtw" or "greedy"
    float score_fine_node_length = 0.25f;       // Fine alignment resolution
    int s_time_div = 16;                         // Score time division
    int p_time_div = 16;                         // Performance time division
    float sfuzziness = 4.0f;                     // Score window margin
    float pfuzziness = 4.0f;                     // Performance window margin
    int window_size = 1;                         // Window size for cutting
    bool pfuzziness_relative_to_tempo = true;    // Tempo-relative margins
    bool shift_onsets = false;                   // Allow onset shifting
    int cap_combinations = 100;                  // Limit combinatorial search
};
```

## Emscripten Integration

The library is designed to work with emscripten:

```bash
# Build with emscripten
emcmake cmake -DEMSCRIPTEN=ON ..
emmake make

# The resulting .wasm and .js files can be used in web applications
```