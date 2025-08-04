#include <parangonar/matchers.hpp>
#include <parangonar/note.hpp>
#include <iostream>
#include <cassert>

using namespace parangonar;

// Test the functions that would be exposed to WASM
namespace wasm_bindings {

Note create_score_note(float onset_beat, float duration_beat, int pitch, const std::string& id) {
    return Note::score_note(onset_beat, duration_beat, pitch, id);
}

Note create_performance_note(float onset_sec, float duration_sec, int pitch, int velocity, const std::string& id) {
    return Note::performance_note(onset_sec, duration_sec, pitch, velocity, id);
}

AlignmentVector align(const std::vector<Note>& score_notes, 
                     const std::vector<Note>& performance_notes,
                     const AutomaticNoteMatcherConfig& config = AutomaticNoteMatcherConfig{}) {
    AutomaticNoteMatcher matcher(config);
    return matcher(score_notes, performance_notes);
}

AlignmentVector match(const std::vector<Note>& score_notes, 
                     const std::vector<Note>& performance_notes) {
    AutomaticNoteMatcher matcher;
    return matcher(score_notes, performance_notes);
}

} // namespace wasm_bindings

void test_wasm_api() {
    std::cout << "Testing WASM API functions..." << std::endl;
    
    // Create test score notes using the WASM API
    std::vector<Note> score_notes;
    score_notes.push_back(wasm_bindings::create_score_note(0.0f, 0.5f, 60, "s1"));
    score_notes.push_back(wasm_bindings::create_score_note(0.5f, 0.5f, 62, "s2"));
    score_notes.push_back(wasm_bindings::create_score_note(1.0f, 0.5f, 64, "s3"));
    score_notes.push_back(wasm_bindings::create_score_note(1.5f, 0.5f, 65, "s4"));
    
    // Create test performance notes using the WASM API
    std::vector<Note> performance_notes;
    performance_notes.push_back(wasm_bindings::create_performance_note(0.1f, 0.4f, 60, 70, "p1"));
    performance_notes.push_back(wasm_bindings::create_performance_note(0.6f, 0.4f, 62, 75, "p2"));
    performance_notes.push_back(wasm_bindings::create_performance_note(1.1f, 0.4f, 64, 80, "p3"));
    performance_notes.push_back(wasm_bindings::create_performance_note(1.6f, 0.4f, 65, 85, "p4"));
    
    assert(score_notes.size() == 4);
    assert(performance_notes.size() == 4);
    
    std::cout << "Created " << score_notes.size() << " score notes and " 
              << performance_notes.size() << " performance notes" << std::endl;
    
    // Test the simple match function
    auto alignment = wasm_bindings::match(score_notes, performance_notes);
    
    std::cout << "Simple match produced " << alignment.size() << " alignments" << std::endl;
    
    // Count alignment types
    int matches = 0, insertions = 0, deletions = 0;
    for (const auto& align : alignment) {
        switch (align.label) {
            case Alignment::Label::MATCH:
                matches++;
                std::cout << "  MATCH: " << align.score_id << " -> " << align.performance_id << std::endl;
                break;
            case Alignment::Label::INSERTION:
                insertions++;
                std::cout << "  INSERTION: -> " << align.performance_id << std::endl;
                break;
            case Alignment::Label::DELETION:
                deletions++;
                std::cout << "  DELETION: " << align.score_id << " ->" << std::endl;
                break;
        }
    }
    
    std::cout << "Matches: " << matches << ", Insertions: " << insertions 
              << ", Deletions: " << deletions << std::endl;
    
    // Test with custom config
    AutomaticNoteMatcherConfig config;
    config.sfuzziness = 2.0f;
    config.pfuzziness = 2.0f;
    config.cap_combinations = 50;
    
    auto alignment_custom = wasm_bindings::align(score_notes, performance_notes, config);
    
    std::cout << "Custom config alignment produced " << alignment_custom.size() << " alignments" << std::endl;
    
    // Basic sanity checks
    assert(alignment.size() > 0);
    assert(alignment_custom.size() > 0);
    assert(matches <= 4); // Can't match more notes than we have
    
    std::cout << "WASM API tests passed!" << std::endl;
}

int main() {
    std::cout << "Testing WASM bindings API..." << std::endl;
    
    try {
        test_wasm_api();
        std::cout << std::endl << "All WASM API tests passed!" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}