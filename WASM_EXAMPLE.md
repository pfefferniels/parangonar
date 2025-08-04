# Parangonar WASM Web Example

This directory contains a complete HTML example demonstrating how to use the Parangonar WebAssembly module for musical note alignment in a web browser.

## Running the Example

### Prerequisites

1. Build the WASM files first:
   ```bash
   mkdir build_emscripten
   cd build_emscripten
   emcmake cmake ..
   emmake make
   ```

2. Copy the generated files to the root directory:
   ```bash
   cp build_emscripten/parangonar.js .
   cp build_emscripten/parangonar.wasm .
   ```

### Local Testing

Since browsers enforce CORS restrictions, you cannot open the HTML file directly. Start a local web server:

```bash
python3 -m http.server 8000
```

Then open your browser and navigate to `http://localhost:8000/example.html`.

## Usage

1. **Add Notes**: The example comes with pre-filled sample notes. You can modify them or add new ones using the "Add Score Note" and "Add Performance Note" buttons.

2. **Configure Alignment**: Adjust the alignment parameters:
   - **Score Fuzziness**: Controls tolerance for score note matching
   - **Performance Fuzziness**: Controls tolerance for performance note matching  
   - **Window Size**: Sets the DTW window size

3. **Perform Alignment**: Click "Perform Note Alignment" to run the alignment algorithm.

4. **View Results**: The results show:
   - Total number of alignments found
   - Detailed list of each alignment with type (MATCH, INSERTION, DELETION)
   - Score note ID ↔ Performance note ID mappings

## Example Data

The example includes sample data with three note pairs:
- Score: C4, E4, G4 (pitches 60, 64, 67) at beats 0, 1, 2
- Performance: Same pitches at times 0.1s, 1.1s, 2.2s with slight timing variations

This demonstrates a typical scenario where the performance has small timing deviations from the written score.

## JavaScript API Usage

The example demonstrates the key WASM API calls:

```javascript
// Load the module
const moduleConfig = {
    onRuntimeInitialized: function() {
        // Module ready
    }
};
ParangonarModule(moduleConfig);

// Create notes
const scoreNote = Module.createScoreNote(onset_beat, duration_beat, pitch, id);
const perfNote = Module.createPerformanceNote(onset_sec, duration_sec, pitch, velocity, id);

// Create note arrays
const scoreNotes = new Module.NoteArray();
scoreNotes.push_back(scoreNote);

const perfNotes = new Module.NoteArray();
perfNotes.push_back(perfNote);

// Configure alignment
const config = new Module.AutomaticNoteMatcherConfig();
config.sfuzziness = 2.0;
config.pfuzziness = 2.0;

// Perform alignment
const alignment = Module.align(scoreNotes, perfNotes, config);

// Process results
for (let i = 0; i < alignment.size(); i++) {
    const align = alignment.get(i);
    console.log(align.label, align.score_id, align.performance_id);
}

// Clean up
alignment.delete();
scoreNotes.delete();
perfNotes.delete();
config.delete();
```

## Notes

- The WASM module loads asynchronously, so wait for the `onRuntimeInitialized` callback
- Always call `.delete()` on WASM objects to prevent memory leaks
- The module supports both `align()` (with config) and `match()` (default config) functions
- Alignment results include MATCH, INSERTION (extra performance note), and DELETION (missing performance note)