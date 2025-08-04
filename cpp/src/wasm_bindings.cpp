#include <parangonar/matchers.hpp>
#include <parangonar/note.hpp>
#include <emscripten/bind.h>
#include <emscripten/val.h>

using namespace emscripten;
using namespace parangonar;

namespace wasm_bindings {

/**
 * Simple function to create a score note from JS values
 */
Note create_score_note(float onset_beat, float duration_beat, int pitch, const std::string& id) {
    return Note::score_note(onset_beat, duration_beat, pitch, id);
}

/**
 * Simple function to create a performance note from JS values
 */
Note create_performance_note(float onset_sec, float duration_sec, int pitch, int velocity, const std::string& id) {
    return Note::performance_note(onset_sec, duration_sec, pitch, velocity, id);
}

/**
 * Main alignment function that can be called from JavaScript
 * Takes arrays of notes and optional config, returns alignment result
 */
AlignmentVector align(const std::vector<Note>& score_notes, 
                     const std::vector<Note>& performance_notes,
                     const AutomaticNoteMatcherConfig& config = AutomaticNoteMatcherConfig{}) {
    AutomaticNoteMatcher matcher(config);
    return matcher(score_notes, performance_notes);
}

/**
 * Simplified alignment function with default config
 */
AlignmentVector match(const std::vector<Note>& score_notes, 
                     const std::vector<Note>& performance_notes) {
    AutomaticNoteMatcher matcher;
    return matcher(score_notes, performance_notes);
}

} // namespace wasm_bindings

// Simple main function for WASM executable
int main() {
    // This function is not really used when called from JS
    return 0;
}

// Emscripten bindings
EMSCRIPTEN_BINDINGS(parangonar) {
    // Register the Note class
    class_<Note>("Note")
        .constructor<>()
        .property("onset_beat", &Note::onset_beat)
        .property("duration_beat", &Note::duration_beat)
        .property("onset_quarter", &Note::onset_quarter)
        .property("duration_quarter", &Note::duration_quarter)
        .property("onset_div", &Note::onset_div)
        .property("duration_div", &Note::duration_div)
        .property("onset_sec", &Note::onset_sec)
        .property("duration_sec", &Note::duration_sec)
        .property("onset_tick", &Note::onset_tick)
        .property("duration_tick", &Note::duration_tick)
        .property("velocity", &Note::velocity)
        .property("track", &Note::track)
        .property("channel", &Note::channel)
        .property("pitch", &Note::pitch)
        .property("voice", &Note::voice)
        .property("id", &Note::id)
        .property("divs_pq", &Note::divs_pq);
    
    // Register note creation helper functions
    function("createScoreNote", &wasm_bindings::create_score_note);
    function("createPerformanceNote", &wasm_bindings::create_performance_note);
    
    // Register the configuration class
    class_<AutomaticNoteMatcherConfig>("AutomaticNoteMatcherConfig")
        .constructor<>()
        .property("alignment_type", &AutomaticNoteMatcherConfig::alignment_type)
        .property("score_fine_node_length", &AutomaticNoteMatcherConfig::score_fine_node_length)
        .property("s_time_div", &AutomaticNoteMatcherConfig::s_time_div)
        .property("p_time_div", &AutomaticNoteMatcherConfig::p_time_div)
        .property("sfuzziness", &AutomaticNoteMatcherConfig::sfuzziness)
        .property("pfuzziness", &AutomaticNoteMatcherConfig::pfuzziness)
        .property("window_size", &AutomaticNoteMatcherConfig::window_size)
        .property("pfuzziness_relative_to_tempo", &AutomaticNoteMatcherConfig::pfuzziness_relative_to_tempo)
        .property("shift_onsets", &AutomaticNoteMatcherConfig::shift_onsets)
        .property("cap_combinations", &AutomaticNoteMatcherConfig::cap_combinations);
    
    // Register the Alignment enum and class
    enum_<Alignment::Label>("AlignmentLabel")
        .value("MATCH", Alignment::Label::MATCH)
        .value("INSERTION", Alignment::Label::INSERTION)
        .value("DELETION", Alignment::Label::DELETION);
    
    class_<Alignment>("Alignment")
        .constructor<Alignment::Label, const std::string&, const std::string&>()
        .property("label", &Alignment::label)
        .property("score_id", &Alignment::score_id)
        .property("performance_id", &Alignment::performance_id);
    
    // Register vector types for arrays
    register_vector<Note>("NoteArray");
    register_vector<Alignment>("AlignmentVector");
    
    // Register the main alignment functions
    function("align", &wasm_bindings::align);
    function("match", &wasm_bindings::match);
}