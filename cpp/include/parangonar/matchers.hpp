#pragma once

#include <parangonar/note.hpp>
#include <parangonar/dtw.hpp>
#include <parangonar/preprocessors.hpp>
#include <memory>

namespace parangonar {

/**
 * Simple greedy matcher for comparison
 */
class SimplestGreedyMatcher {
public:
    AlignmentVector operator()(const NoteArray& score_notes, const NoteArray& performance_notes);
};

/**
 * Sequence augmented greedy matcher with combinatorial optimization
 */
class SequenceAugmentedGreedyMatcher {
private:
    bool overlap = false;
    
public:
    AlignmentVector operator()(
        const NoteArray& score_notes,
        const NoteArray& performance_notes,
        const TimeAlignmentVector& alignment_times,
        bool shift = false,
        int cap_combinations = 10000
    );
    
private:
    struct CombinationResult {
        double score;
        std::vector<size_t> omit_indices;
    };
    
    CombinationResult find_best_combination(
        const std::vector<float>& long_times,
        const std::vector<float>& short_times,
        bool shift,
        int cap_combinations
    );
};

/**
 * Configuration structure for AutomaticNoteMatcher
 */
struct AutomaticNoteMatcherConfig {
    std::string alignment_type = "dtw";
    float score_fine_node_length = 0.25f;
    int s_time_div = 16;
    int p_time_div = 16;
    float sfuzziness = 4.0f;
    float pfuzziness = 4.0f;
    int window_size = 1;
    bool pfuzziness_relative_to_tempo = true;
    bool shift_onsets = false;
    int cap_combinations = 10000;
};

/**
 * Main automatic note matcher - equivalent to Python's PianoRollNoNodeMatcher
 */
class AutomaticNoteMatcher {
private:
    std::unique_ptr<DynamicTimeWarping> note_matcher_;
    std::unique_ptr<SequenceAugmentedGreedyMatcher> symbolic_note_matcher_;
    std::unique_ptr<SimplestGreedyMatcher> greedy_symbolic_note_matcher_;
    
    // Configuration parameters
    std::string alignment_type_ = "dtw";
    float score_fine_node_length_ = 0.25f;
    int s_time_div_ = 16;
    int p_time_div_ = 16;
    float sfuzziness_ = 8.0f;
    float pfuzziness_ = 8.0f;
    int window_size_ = 1;
    bool pfuzziness_relative_to_tempo_ = true;
    bool shift_onsets_ = false;
    int cap_combinations_ = 10000;
    
public:
    using Config = AutomaticNoteMatcherConfig;
    
    AutomaticNoteMatcher();
    explicit AutomaticNoteMatcher(const Config& config);
    
    // Main alignment function
    AlignmentVector operator()(const NoteArray& score_notes, 
                              const NoteArray& performance_notes,
                              bool verbose_time = false);
    
    // Getter methods for configuration
    const Config& get_config() const;
    void set_config(const Config& config);
    
private:
    void initialize_matchers();
    void update_config(const Config& config);
};

/**
 * Evaluation functions
 */
namespace evaluation {

struct FScoreResult {
    double precision;
    double recall;
    double f_score;
    size_t n_predicted;
    size_t n_ground_truth;
};

FScoreResult fscore_alignments(
    const AlignmentVector& prediction,
    const AlignmentVector& ground_truth,
    const std::vector<Alignment::Label>& types,
    bool return_numbers = false
);

// Helper function to evaluate only matches
FScoreResult fscore_matches(
    const AlignmentVector& prediction,
    const AlignmentVector& ground_truth
);

} // namespace evaluation

} // namespace parangonar