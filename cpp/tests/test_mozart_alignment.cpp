#include <parangonar/matchers.hpp>
#include <parangonar/note.hpp>
#include <parangonar/match_parser.hpp>
#include <iostream>
#include <cassert>
#include <filesystem>
#include <string>

using namespace parangonar;

/**
 * Test that loads the Mozart K265 Variation 1 match file and performs
 * comprehensive alignment testing to identify potential issues with longer alignments.
 */
class MozartAlignmentTest {
public:
    MozartAlignmentTest() = default;
    
    void run_all_tests() {
        std::cout << "=== Mozart K265 Variation 1 Alignment Test ===" << std::endl;
        
        try {
            load_mozart_data();
            test_data_quality();
            test_simple_greedy_matcher();
            test_automatic_note_matcher();
            analyze_alignment_challenges();
            
            std::cout << "\n=== All Mozart tests completed successfully! ===" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Mozart test failed: " << e.what() << std::endl;
            throw;
        }
    }
    
private:
    MatchFileData mozart_data;
    NoteArray score_notes;
    NoteArray performance_notes;
    AlignmentVector ground_truth_alignment;
    
    void load_mozart_data() {
        std::cout << "\n--- Loading Mozart K265 Var1 Match Data ---" << std::endl;
        
        // Try different possible paths for the match file
        std::vector<std::string> possible_paths = {
            "../test_data/mozart_k265_var1.match",
            "test_data/mozart_k265_var1.match",
            "../../test_data/mozart_k265_var1.match",
            "mozart_k265_var1.match"
        };
        
        std::string match_file_path;
        for (const auto& path : possible_paths) {
            if (std::filesystem::exists(path)) {
                match_file_path = path;
                break;
            }
        }
        
        if (match_file_path.empty()) {
            throw std::runtime_error("Cannot find mozart_k265_var1.match file in any of the expected locations");
        }
        
        std::cout << "Loading match file: " << match_file_path << std::endl;
        
        // Parse the match file
        mozart_data = MatchFileParser::parse_file(match_file_path);
        
        // Convert to note arrays
        auto note_arrays = MatchFileParser::to_note_arrays(mozart_data);
        score_notes = note_arrays.first;
        performance_notes = note_arrays.second;
        
        // Get ground truth alignment
        ground_truth_alignment = MatchFileParser::to_alignment(mozart_data);
        
        std::cout << "Match file info:" << std::endl;
        std::cout << "  Version: " << mozart_data.info.version << std::endl;
        std::cout << "  MIDI Clock Units: " << mozart_data.info.midi_clock_units << std::endl;
        std::cout << "  MIDI Clock Rate: " << mozart_data.info.midi_clock_rate << std::endl;
        std::cout << "  Key Signature: " << mozart_data.info.key_signature << std::endl;
        std::cout << "  Time Signature: " << mozart_data.info.time_signature << std::endl;
        std::cout << "  Score notes: " << score_notes.size() << std::endl;
        std::cout << "  Performance notes: " << performance_notes.size() << std::endl;
        std::cout << "  Alignment entries: " << ground_truth_alignment.size() << std::endl;
        std::cout << "  Sustain pedal events: " << mozart_data.sustain_pedal.size() << std::endl;
    }
    
    void test_data_quality() {
        std::cout << "\n--- Testing Data Quality ---" << std::endl;
        
        // Check that we have reasonable amounts of data
        assert(score_notes.size() > 50); // Mozart variation should have many notes
        assert(performance_notes.size() > 50);
        assert(ground_truth_alignment.size() > 50);
        
        // Check score note properties
        std::cout << "Score notes analysis:" << std::endl;
        float min_onset = score_notes[0].onset_beat;
        float max_onset = score_notes[0].onset_beat;
        int min_pitch = score_notes[0].pitch;
        int max_pitch = score_notes[0].pitch;
        
        for (const auto& note : score_notes) {
            min_onset = std::min(min_onset, note.onset_beat);
            max_onset = std::max(max_onset, note.onset_beat);
            min_pitch = std::min(min_pitch, note.pitch);
            max_pitch = std::max(max_pitch, note.pitch);
            
            // Validate note properties
            assert(note.pitch >= 0 && note.pitch <= 127);
            assert(note.onset_beat >= 0);
            assert(note.duration_beat >= 0);
            assert(!note.id.empty());
        }
        
        std::cout << "  Onset range: " << min_onset << " - " << max_onset << " beats" << std::endl;
        std::cout << "  Pitch range: " << min_pitch << " - " << max_pitch << " (MIDI)" << std::endl;
        std::cout << "  Duration: " << (max_onset - min_onset) << " beats" << std::endl;
        
        // Check performance note properties
        std::cout << "Performance notes analysis:" << std::endl;
        float min_onset_sec = performance_notes[0].onset_sec;
        float max_onset_sec = performance_notes[0].onset_sec;
        int min_velocity = performance_notes[0].velocity;
        int max_velocity = performance_notes[0].velocity;
        
        for (const auto& note : performance_notes) {
            min_onset_sec = std::min(min_onset_sec, note.onset_sec);
            max_onset_sec = std::max(max_onset_sec, note.onset_sec);
            min_velocity = std::min(min_velocity, note.velocity);
            max_velocity = std::max(max_velocity, note.velocity);
            
            // Validate note properties
            assert(note.pitch >= 0 && note.pitch <= 127);
            assert(note.onset_sec >= 0);
            assert(note.duration_sec >= 0);
            assert(note.velocity >= 0 && note.velocity <= 127);
            assert(!note.id.empty());
        }
        
        std::cout << "  Onset range: " << min_onset_sec << " - " << max_onset_sec << " seconds" << std::endl;
        std::cout << "  Duration: " << (max_onset_sec - min_onset_sec) << " seconds" << std::endl;
        std::cout << "  Velocity range: " << min_velocity << " - " << max_velocity << std::endl;
        
        // Analyze ground truth alignment
        int matches = 0, insertions = 0, deletions = 0;
        for (const auto& align : ground_truth_alignment) {
            switch (align.label) {
                case Alignment::Label::MATCH: matches++; break;
                case Alignment::Label::INSERTION: insertions++; break;
                case Alignment::Label::DELETION: deletions++; break;
            }
        }
        
        std::cout << "Ground truth alignment:" << std::endl;
        std::cout << "  Matches: " << matches << std::endl;
        std::cout << "  Insertions: " << insertions << std::endl;
        std::cout << "  Deletions: " << deletions << std::endl;
        
        assert(matches > 0); // Should have at least some matches
        
        std::cout << "Data quality tests passed!" << std::endl;
    }
    
    void test_simple_greedy_matcher() {
        std::cout << "\n--- Testing SimplestGreedyMatcher on Mozart Data ---" << std::endl;
        
        SimplestGreedyMatcher matcher;
        auto predicted_alignment = matcher(score_notes, performance_notes);
        
        std::cout << "SimplestGreedyMatcher results:" << std::endl;
        std::cout << "  Predicted alignment size: " << predicted_alignment.size() << std::endl;
        
        // Count alignment types
        int matches = 0, insertions = 0, deletions = 0;
        for (const auto& align : predicted_alignment) {
            switch (align.label) {
                case Alignment::Label::MATCH: matches++; break;
                case Alignment::Label::INSERTION: insertions++; break;
                case Alignment::Label::DELETION: deletions++; break;
            }
        }
        
        std::cout << "  Matches: " << matches << std::endl;
        std::cout << "  Insertions: " << insertions << std::endl;
        std::cout << "  Deletions: " << deletions << std::endl;
        
        // Evaluate against ground truth
        auto fscore_result = evaluation::fscore_matches(predicted_alignment, ground_truth_alignment);
        
        std::cout << "F-score evaluation against ground truth:" << std::endl;
        std::cout << "  Precision: " << fscore_result.precision << std::endl;
        std::cout << "  Recall: " << fscore_result.recall << std::endl;
        std::cout << "  F-score: " << fscore_result.f_score << std::endl;
        
        // For a simple greedy matcher, we expect decent but not perfect performance
        // The Mozart piece is complex so F-score might be lower
        std::cout << "SimplestGreedyMatcher test completed!" << std::endl;
    }
    
    void test_automatic_note_matcher() {
        std::cout << "\n--- Testing AutomaticNoteMatcher on Mozart Data ---" << std::endl;
        
        AutomaticNoteMatcher matcher;
        auto predicted_alignment = matcher(score_notes, performance_notes, true);
        
        std::cout << "AutomaticNoteMatcher results:" << std::endl;
        std::cout << "  Predicted alignment size: " << predicted_alignment.size() << std::endl;
        
        // Count alignment types
        int matches = 0, insertions = 0, deletions = 0;
        for (const auto& align : predicted_alignment) {
            switch (align.label) {
                case Alignment::Label::MATCH: matches++; break;
                case Alignment::Label::INSERTION: insertions++; break;
                case Alignment::Label::DELETION: deletions++; break;
            }
        }
        
        std::cout << "  Matches: " << matches << std::endl;
        std::cout << "  Insertions: " << insertions << std::endl;
        std::cout << "  Deletions: " << deletions << std::endl;
        
        // Evaluate against ground truth
        auto fscore_result = evaluation::fscore_matches(predicted_alignment, ground_truth_alignment);
        
        std::cout << "F-score evaluation against ground truth:" << std::endl;
        std::cout << "  Precision: " << fscore_result.precision << std::endl;
        std::cout << "  Recall: " << fscore_result.recall << std::endl;
        std::cout << "  F-score: " << fscore_result.f_score << std::endl;
        
        // AutomaticNoteMatcher should perform better than simple greedy
        // But on complex pieces like Mozart, it might still struggle
        // This will help us identify alignment issues with longer pieces
        if (fscore_result.f_score < 0.8) {
            std::cout << "WARNING: F-score is relatively low (" << fscore_result.f_score 
                      << "). This may indicate alignment issues with longer/complex pieces." << std::endl;
        }
        
        std::cout << "AutomaticNoteMatcher test completed!" << std::endl;
    }
    
    void analyze_alignment_challenges() {
        std::cout << "\n--- Analyzing Alignment Challenges ---" << std::endl;
        
        // Analyze the complexity of the Mozart piece to understand alignment challenges
        
        // 1. Analyze timing variations
        std::cout << "Timing analysis:" << std::endl;
        if (score_notes.size() > 1 && performance_notes.size() > 1) {
            // Calculate average inter-onset intervals
            float score_avg_ioi = 0;
            for (size_t i = 1; i < score_notes.size(); ++i) {
                score_avg_ioi += score_notes[i].onset_beat - score_notes[i-1].onset_beat;
            }
            score_avg_ioi /= (score_notes.size() - 1);
            
            float perf_avg_ioi = 0;
            for (size_t i = 1; i < performance_notes.size(); ++i) {
                perf_avg_ioi += performance_notes[i].onset_sec - performance_notes[i-1].onset_sec;
            }
            perf_avg_ioi /= (performance_notes.size() - 1);
            
            std::cout << "  Average score IOI: " << score_avg_ioi << " beats" << std::endl;
            std::cout << "  Average performance IOI: " << perf_avg_ioi << " seconds" << std::endl;
        }
        
        // 2. Analyze pitch diversity
        auto unique_score_pitches = note_array::unique_pitches(score_notes);
        auto unique_perf_pitches = note_array::unique_pitches(performance_notes);
        
        std::cout << "Pitch diversity:" << std::endl;
        std::cout << "  Unique score pitches: " << unique_score_pitches.size() << std::endl;
        std::cout << "  Unique performance pitches: " << unique_perf_pitches.size() << std::endl;
        
        // 3. Analyze note density
        if (!score_notes.empty() && !performance_notes.empty()) {
            float score_duration = score_notes.back().onset_beat - score_notes.front().onset_beat;
            float perf_duration = performance_notes.back().onset_sec - performance_notes.front().onset_sec;
            
            float score_density = score_notes.size() / score_duration;
            float perf_density = performance_notes.size() / perf_duration;
            
            std::cout << "Note density:" << std::endl;
            std::cout << "  Score density: " << score_density << " notes/beat" << std::endl;
            std::cout << "  Performance density: " << perf_density << " notes/second" << std::endl;
        }
        
        // 4. Check for potential ornaments or complex patterns
        int gt_matches = 0, gt_insertions = 0, gt_deletions = 0;
        for (const auto& align : ground_truth_alignment) {
            switch (align.label) {
                case Alignment::Label::MATCH: gt_matches++; break;
                case Alignment::Label::INSERTION: gt_insertions++; break;
                case Alignment::Label::DELETION: gt_deletions++; break;
            }
        }
        
        float insertion_ratio = static_cast<float>(gt_insertions) / ground_truth_alignment.size();
        float deletion_ratio = static_cast<float>(gt_deletions) / ground_truth_alignment.size();
        
        std::cout << "Ground truth complexity:" << std::endl;
        std::cout << "  Insertion ratio: " << insertion_ratio << std::endl;
        std::cout << "  Deletion ratio: " << deletion_ratio << std::endl;
        
        if (insertion_ratio > 0.1 || deletion_ratio > 0.1) {
            std::cout << "WARNING: High insertion/deletion ratio detected. This piece has complex" << std::endl;
            std::cout << "         ornamentations or timing variations that may challenge alignment algorithms." << std::endl;
        }
        
        std::cout << "Alignment challenge analysis completed!" << std::endl;
    }
};

int main() {
    std::cout << "Starting comprehensive Mozart K265 Variation 1 alignment test..." << std::endl;
    
    try {
        MozartAlignmentTest test;
        test.run_all_tests();
        
        std::cout << "\nAll tests completed successfully!" << std::endl;
        std::cout << "This comprehensive test using real Mozart data helps identify potential" << std::endl;
        std::cout << "alignment issues with longer and more complex musical pieces." << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Test failed with unknown exception" << std::endl;
        return 1;
    }
}