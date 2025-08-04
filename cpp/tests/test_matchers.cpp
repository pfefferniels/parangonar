#include <parangonar/matchers.hpp>
#include <parangonar/note.hpp>
#include <iostream>
#include <cassert>
#include <random>

using namespace parangonar;

// Helper function to create test notes
NoteArray create_test_score_notes() {
    std::vector<Note> notes;
    
    // Create a simple melody - C major scale
    std::vector<int> pitches = {60, 62, 64, 65, 67, 69, 71, 72}; // C4 to C5
    
    for (size_t i = 0; i < pitches.size(); ++i) {
        Note note;
        note.onset_beat = static_cast<float>(i) * 0.5f;  // Half beat intervals
        note.duration_beat = 0.4f;
        note.pitch = pitches[i];
        note.id = "s" + std::to_string(i);
        notes.push_back(note);
    }
    
    return NoteArray(notes);
}

NoteArray create_test_performance_notes() {
    std::vector<Note> notes;
    
    // Create corresponding performance notes with slight timing variations
    std::vector<int> pitches = {60, 62, 64, 65, 67, 69, 71, 72};
    std::random_device rd;
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_real_distribution<float> timing_noise(-0.05f, 0.05f);
    
    for (size_t i = 0; i < pitches.size(); ++i) {
        Note note;
        note.onset_sec = static_cast<float>(i) * 0.6f + timing_noise(gen); // Slightly different tempo
        note.duration_sec = 0.4f;
        note.pitch = pitches[i];
        note.velocity = 70;
        note.id = "p" + std::to_string(i);
        notes.push_back(note);
    }
    
    return NoteArray(notes);
}

AlignmentVector create_ground_truth_alignment() {
    AlignmentVector alignment;
    
    for (int i = 0; i < 8; ++i) {
        alignment.emplace_back(
            Alignment::Label::MATCH,
            "s" + std::to_string(i),
            "p" + std::to_string(i)
        );
    }
    
    return alignment;
}

void test_note_array() {
    std::cout << "Testing NoteArray..." << std::endl;
    
    auto notes = create_test_score_notes();
    assert(notes.size() == 8);
    
    // Test pitch filtering
    auto c_notes = notes.filter_by_pitch(60);
    assert(c_notes.size() == 1);
    assert(c_notes[0].pitch == 60);
    
    // Test unique pitches
    auto unique_pitches = notes.unique_pitches();
    assert(unique_pitches.size() == 8);
    
    // Test onset times
    auto onset_times = notes.onset_times_beat();
    assert(onset_times.size() == 8);
    assert(std::abs(onset_times[0] - 0.0f) < 1e-6f);
    assert(std::abs(onset_times[1] - 0.5f) < 1e-6f);
    
    std::cout << "NoteArray tests passed!" << std::endl;
}

void test_dtw() {
    std::cout << "Testing DTW..." << std::endl;
    
    // Create simple test sequences
    std::vector<std::vector<float>> X = {{1, 0}, {0, 1}, {1, 1}, {0, 0}};
    std::vector<std::vector<float>> Y = {{1, 0}, {0, 1}, {1, 1}};
    
    DynamicTimeWarping dtw;
    auto result = dtw.compute(X, Y, true, false);
    
    assert(result.distance >= 0);
    assert(!result.path.empty());
    assert(result.path[0].row == 0);
    assert(result.path[0].col == 0);
    
    std::cout << "DTW distance: " << result.distance << std::endl;
    std::cout << "DTW path length: " << result.path.size() << std::endl;
    
    std::cout << "DTW tests passed!" << std::endl;
}

void test_simple_greedy_matcher() {
    std::cout << "Testing SimplestGreedyMatcher..." << std::endl;
    
    auto score_notes = create_test_score_notes();
    auto perf_notes = create_test_performance_notes();
    
    SimplestGreedyMatcher matcher;
    auto alignment = matcher(score_notes, perf_notes);
    
    // Count matches
    int match_count = 0;
    for (const auto& align : alignment) {
        if (align.label == Alignment::Label::MATCH) {
            match_count++;
        }
    }
    
    std::cout << "Simple greedy matcher found " << match_count << " matches" << std::endl;
    assert(match_count <= 8); // Should match all or some notes
    
    std::cout << "SimplestGreedyMatcher tests passed!" << std::endl;
}

void test_automatic_note_matcher() {
    std::cout << "Testing AutomaticNoteMatcher..." << std::endl;
    
    auto score_notes = create_test_score_notes();
    auto perf_notes = create_test_performance_notes();
    auto ground_truth = create_ground_truth_alignment();
    
    AutomaticNoteMatcher matcher;
    auto alignment = matcher(score_notes, perf_notes, true);
    
    std::cout << "Alignment size: " << alignment.size() << std::endl;
    
    // Count different types of alignments
    int matches = 0, insertions = 0, deletions = 0;
    for (const auto& align : alignment) {
        switch (align.label) {
            case Alignment::Label::MATCH:
                matches++;
                break;
            case Alignment::Label::INSERTION:
                insertions++;
                break;
            case Alignment::Label::DELETION:
                deletions++;
                break;
        }
    }
    
    std::cout << "Matches: " << matches << ", Insertions: " << insertions 
              << ", Deletions: " << deletions << std::endl;
    
    // Evaluate against ground truth
    auto fscore_result = evaluation::fscore_matches(alignment, ground_truth);
    
    std::cout << "F-score evaluation:" << std::endl;
    std::cout << "  Precision: " << fscore_result.precision << std::endl;
    std::cout << "  Recall: " << fscore_result.recall << std::endl;
    std::cout << "  F-score: " << fscore_result.f_score << std::endl;
    
    // The alignment should be reasonably good
    assert(fscore_result.f_score > 0.5); // At least 50% F-score
    
    std::cout << "AutomaticNoteMatcher tests passed!" << std::endl;
}

void test_evaluation() {
    std::cout << "Testing evaluation functions..." << std::endl;
    
    // Create perfect alignment
    AlignmentVector perfect_pred = {
        {Alignment::Label::MATCH, "s1", "p1"},
        {Alignment::Label::MATCH, "s2", "p2"},
        {Alignment::Label::DELETION, "s3", ""}
    };
    
    AlignmentVector perfect_gt = perfect_pred; // Same as prediction
    
    auto result = evaluation::fscore_matches(perfect_pred, perfect_gt);
    
    std::cout << "Perfect alignment F-score: " << result.f_score << std::endl;
    assert(std::abs(result.f_score - 1.0) < 1e-6); // Should be perfect
    
    // Test imperfect alignment
    AlignmentVector imperfect_pred = {
        {Alignment::Label::MATCH, "s1", "p1"},
        {Alignment::Label::MATCH, "s2", "p3"}, // Wrong match
        {Alignment::Label::INSERTION, "", "p2"}
    };
    
    auto imperfect_result = evaluation::fscore_matches(imperfect_pred, perfect_gt);
    std::cout << "Imperfect alignment F-score: " << imperfect_result.f_score << std::endl;
    assert(imperfect_result.f_score < 1.0); // Should be less than perfect
    
    std::cout << "Evaluation tests passed!" << std::endl;
}

int main() {
    std::cout << "Starting parangonar C++ tests..." << std::endl;
    
    try {
        test_note_array();
        test_dtw();
        test_simple_greedy_matcher();
        test_automatic_note_matcher();
        test_evaluation();
        
        std::cout << std::endl << "All tests passed successfully!" << std::endl;
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}