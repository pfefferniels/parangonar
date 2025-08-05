#include <parangonar/matchers.hpp>
#include <algorithm>
#include <set>
#include <random>
#include <cmath>
#include <iostream>
#include <chrono>

namespace parangonar {

// SimplestGreedyMatcher implementation
AlignmentVector SimplestGreedyMatcher::operator()(
    const NoteArray& score_notes, 
    const NoteArray& performance_notes) {
    
    AlignmentVector alignment;
    std::set<std::string> performance_aligned;
    std::set<std::string> score_aligned;
    
    for (const auto& score_note : score_notes) {
        std::string performance_id;
        bool found_match = false;
        
        // Find matching pitches
        for (const auto& perf_note : performance_notes) {
            if (score_note.pitch == perf_note.pitch &&
                performance_aligned.find(perf_note.id) == performance_aligned.end()) {
                performance_id = perf_note.id;
                performance_aligned.insert(perf_note.id);
                score_aligned.insert(score_note.id);
                found_match = true;
                break;
            }
        }
        
        if (found_match) {
            alignment.emplace_back(Alignment::Label::MATCH, score_note.id, performance_id);
        } else {
            alignment.emplace_back(Alignment::Label::DELETION, score_note.id);
        }
    }
    
    // Add unaligned performance notes as insertions
    for (const auto& perf_note : performance_notes) {
        if (performance_aligned.find(perf_note.id) == performance_aligned.end()) {
            alignment.emplace_back(Alignment::Label::INSERTION, "", perf_note.id);
        }
    }
    
    return alignment;
}

// SequenceAugmentedGreedyMatcher implementation
SequenceAugmentedGreedyMatcher::CombinationResult SequenceAugmentedGreedyMatcher::find_best_combination(
    const std::vector<float>& long_times,
    const std::vector<float>& short_times,
    bool shift,
    int cap_combinations) {
    
    const size_t n_long = long_times.size();
    const size_t n_short = short_times.size();
    const size_t extra_notes = n_long - n_short;
    
    if (extra_notes == 0) {
        return {0.0, {}};
    }
    
    CombinationResult best_result;
    best_result.score = std::numeric_limits<double>::infinity();
    
    // Generate combinations to try
    std::vector<std::vector<size_t>> combinations;
    
    if (cap_combinations > 0) {
        // Calculate total number of combinations
        double total_combinations = 1.0;
        for (size_t i = 0; i < extra_notes; ++i) {
            total_combinations *= static_cast<double>(n_long - i) / static_cast<double>(i + 1);
        }
        
        if (total_combinations > cap_combinations) {
            // Random sampling
            std::random_device rd;
            std::mt19937 gen(rd());
            
            for (int i = 0; i < cap_combinations; ++i) {
                std::vector<size_t> combination;
                std::vector<size_t> available_indices(n_long);
                std::iota(available_indices.begin(), available_indices.end(), 0);
                
                for (size_t j = 0; j < extra_notes; ++j) {
                    std::uniform_int_distribution<size_t> dist(0, available_indices.size() - 1);
                    size_t chosen_idx = dist(gen);
                    combination.push_back(available_indices[chosen_idx]);
                    available_indices.erase(available_indices.begin() + chosen_idx);
                }
                
                combinations.push_back(combination);
            }
        } else {
            // Generate all combinations
            std::vector<bool> selector(n_long, false);
            std::fill(selector.end() - extra_notes, selector.end(), true);
            
            do {
                std::vector<size_t> combination;
                for (size_t i = 0; i < n_long; ++i) {
                    if (selector[i]) {
                        combination.push_back(i);
                    }
                }
                combinations.push_back(combination);
            } while (std::next_permutation(selector.begin(), selector.end()));
        }
    }
    
    // Evaluate each combination
    for (const auto& omit_indices : combinations) {
        std::vector<float> shortened_times;
        
        // Create shortened version by omitting specified indices
        std::set<size_t> omit_set(omit_indices.begin(), omit_indices.end());
        for (size_t i = 0; i < n_long; ++i) {
            if (omit_set.find(i) == omit_set.end()) {
                shortened_times.push_back(long_times[i]);
            }
        }
        
        // Calculate score
        double score = 0.0;
        
        if (shift && shortened_times.size() == short_times.size()) {
            // Calculate optimal shift
            double sum_diff = 0.0;
            for (size_t i = 0; i < shortened_times.size(); ++i) {
                sum_diff += shortened_times[i] - short_times[i];
            }
            double optimal_shift = sum_diff / shortened_times.size();
            
            // Calculate squared error with optimal shift
            for (size_t i = 0; i < shortened_times.size(); ++i) {
                double diff = shortened_times[i] - short_times[i] - optimal_shift;
                score += diff * diff;
            }
        } else {
            // Calculate squared error without shift
            for (size_t i = 0; i < std::min(shortened_times.size(), short_times.size()); ++i) {
                double diff = shortened_times[i] - short_times[i];
                score += diff * diff;
            }
        }
        
        if (score < best_result.score) {
            best_result.score = score;
            best_result.omit_indices = omit_indices;
        }
    }
    
    return best_result;
}

AlignmentVector SequenceAugmentedGreedyMatcher::operator()(
    const NoteArray& score_notes,
    const NoteArray& performance_notes,
    const TimeAlignmentVector& alignment_times,
    bool shift,
    int cap_combinations) {
    
    AlignmentVector alignment;
    std::set<std::string> performance_aligned;
    
    // Create time interpolator
    std::vector<float> score_times, perf_times;
    for (const auto& align : alignment_times) {
        score_times.push_back(align.score_time);
        perf_times.push_back(align.performance_time);
    }
    
    if (score_times.size() < 2) {
        // Fall back to simple greedy matching
        SimplestGreedyMatcher simple_matcher;
        return simple_matcher(score_notes, performance_notes);
    }
    
    preprocessors::LinearInterpolator interpolator(score_times, perf_times);
    
    // Get unique pitches from score
    auto unique_pitches = note_array::unique_pitches(score_notes);
    
    for (int pitch : unique_pitches) {
        auto score_pitch_notes = note_array::filter_by_pitch(score_notes, pitch);
        auto perf_pitch_notes = note_array::filter_by_pitch(performance_notes, pitch);
        
        if (score_pitch_notes.empty() || perf_pitch_notes.empty()) {
            // Handle empty cases
            for (const auto& note : score_pitch_notes) {
                alignment.emplace_back(Alignment::Label::DELETION, note.id);
            }
            for (const auto& note : perf_pitch_notes) {
                alignment.emplace_back(Alignment::Label::INSERTION, "", note.id);
                performance_aligned.insert(note.id);
            }
            continue;
        }
        
        // Get onset times and sort
        std::vector<float> score_onsets = note_array::onset_times_beat(score_pitch_notes);
        std::vector<float> perf_onsets = note_array::onset_times_sec(perf_pitch_notes);
        
        // Convert score onsets to performance time domain
        auto score_onsets_converted = interpolator.interpolate(score_onsets);
        
        // Sort by onset times
        std::vector<size_t> score_indices(score_onsets.size());
        std::iota(score_indices.begin(), score_indices.end(), 0);
        std::sort(score_indices.begin(), score_indices.end(),
                  [&score_onsets_converted](size_t i, size_t j) {
                      return score_onsets_converted[i] < score_onsets_converted[j];
                  });
        
        std::vector<size_t> perf_indices(perf_onsets.size());
        std::iota(perf_indices.begin(), perf_indices.end(), 0);
        std::sort(perf_indices.begin(), perf_indices.end(),
                  [&perf_onsets](size_t i, size_t j) {
                      return perf_onsets[i] < perf_onsets[j];
                  });
        
        // Create sorted onset vectors
        std::vector<float> sorted_score_onsets, sorted_perf_onsets;
        for (size_t idx : score_indices) {
            sorted_score_onsets.push_back(score_onsets_converted[idx]);
        }
        for (size_t idx : perf_indices) {
            sorted_perf_onsets.push_back(perf_onsets[idx]);
        }
        
        const size_t score_count = sorted_score_onsets.size();
        const size_t perf_count = sorted_perf_onsets.size();
        const size_t common_count = std::min(score_count, perf_count);
        
        if (score_count == perf_count) {
            // Equal number of notes - align all
            for (size_t i = 0; i < common_count; ++i) {
                const auto& score_note = score_pitch_notes[score_indices[i]];
                const auto& perf_note = perf_pitch_notes[perf_indices[i]];
                alignment.emplace_back(Alignment::Label::MATCH, score_note.id, perf_note.id);
                performance_aligned.insert(perf_note.id);
            }
        } else {
            // Different number of notes - find best combination
            bool score_longer = score_count > perf_count;
            
            auto best_combination = find_best_combination(
                score_longer ? sorted_score_onsets : sorted_perf_onsets,
                score_longer ? sorted_perf_onsets : sorted_score_onsets,
                shift,
                cap_combinations
            );
            
            std::set<size_t> omit_set(best_combination.omit_indices.begin(),
                                     best_combination.omit_indices.end());
            
            if (score_longer) {
                // Score has more notes
                size_t perf_idx = 0;
                for (size_t score_idx = 0; score_idx < score_count; ++score_idx) {
                    const auto& score_note = score_pitch_notes[score_indices[score_idx]];
                    
                    if (omit_set.find(score_idx) == omit_set.end() && perf_idx < perf_count) {
                        // Align this score note
                        const auto& perf_note = perf_pitch_notes[perf_indices[perf_idx]];
                        alignment.emplace_back(Alignment::Label::MATCH, score_note.id, perf_note.id);
                        performance_aligned.insert(perf_note.id);
                        perf_idx++;
                    } else {
                        // Delete this score note
                        alignment.emplace_back(Alignment::Label::DELETION, score_note.id);
                    }
                }
            } else {
                // Performance has more notes
                size_t score_idx = 0;
                for (size_t perf_idx = 0; perf_idx < perf_count; ++perf_idx) {
                    const auto& perf_note = perf_pitch_notes[perf_indices[perf_idx]];
                    
                    if (omit_set.find(perf_idx) == omit_set.end() && score_idx < score_count) {
                        // Align this performance note
                        const auto& score_note = score_pitch_notes[score_indices[score_idx]];
                        alignment.emplace_back(Alignment::Label::MATCH, score_note.id, perf_note.id);
                        performance_aligned.insert(perf_note.id);
                        score_idx++;
                    } else {
                        // Insert this performance note
                        alignment.emplace_back(Alignment::Label::INSERTION, "", perf_note.id);
                        performance_aligned.insert(perf_note.id);
                    }
                }
            }
        }
    }
    
    // Add any unaligned performance notes as insertions
    for (const auto& perf_note : performance_notes) {
        if (performance_aligned.find(perf_note.id) == performance_aligned.end()) {
            alignment.emplace_back(Alignment::Label::INSERTION, "", perf_note.id);
        }
    }
    
    return alignment;
}

// AutomaticNoteMatcher implementation
AutomaticNoteMatcher::AutomaticNoteMatcher() {
    initialize_matchers();
}

AutomaticNoteMatcher::AutomaticNoteMatcher(const Config& config) {
    update_config(config);
    initialize_matchers();
}

void AutomaticNoteMatcher::initialize_matchers() {
    note_matcher_ = std::make_unique<DynamicTimeWarping>();
    symbolic_note_matcher_ = std::make_unique<SequenceAugmentedGreedyMatcher>();
    greedy_symbolic_note_matcher_ = std::make_unique<SimplestGreedyMatcher>();
}

void AutomaticNoteMatcher::update_config(const Config& config) {
    alignment_type_ = config.alignment_type;
    score_fine_node_length_ = config.score_fine_node_length;
    s_time_div_ = config.s_time_div;
    p_time_div_ = config.p_time_div;
    sfuzziness_ = config.sfuzziness;
    pfuzziness_ = config.pfuzziness;
    window_size_ = config.window_size;
    pfuzziness_relative_to_tempo_ = config.pfuzziness_relative_to_tempo;
    shift_onsets_ = config.shift_onsets;
    cap_combinations_ = config.cap_combinations;
}

const AutomaticNoteMatcher::Config& AutomaticNoteMatcher::get_config() const {
    static Config config;
    config.alignment_type = alignment_type_;
    config.score_fine_node_length = score_fine_node_length_;
    config.s_time_div = s_time_div_;
    config.p_time_div = p_time_div_;
    config.sfuzziness = sfuzziness_;
    config.pfuzziness = pfuzziness_;
    config.window_size = window_size_;
    config.pfuzziness_relative_to_tempo = pfuzziness_relative_to_tempo_;
    config.shift_onsets = shift_onsets_;
    config.cap_combinations = cap_combinations_;
    return config;
}

void AutomaticNoteMatcher::set_config(const Config& config) {
    update_config(config);
}

AlignmentVector AutomaticNoteMatcher::operator()(
    const NoteArray& score_notes,
    const NoteArray& performance_notes,
    bool verbose_time) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Step 1: Initial coarse DTW pass
    auto dtw_alignment_times_init = preprocessors::alignment_times_from_dtw(
        score_notes, performance_notes, *note_matcher_, 4.0f, s_time_div_, p_time_div_
    );
    
    auto t1 = std::chrono::high_resolution_clock::now();
    if (verbose_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - start_time);
        std::cout << duration.count() / 1000.0 << " sec : Initial coarse DTW pass" << std::endl;
    }
    
    // Step 2: Cut arrays into windows
    auto [score_note_arrays, performance_note_arrays] = preprocessors::cut_note_arrays(
        performance_notes, score_notes, dtw_alignment_times_init,
        sfuzziness_, pfuzziness_, window_size_, pfuzziness_relative_to_tempo_
    );
    
    auto t2 = std::chrono::high_resolution_clock::now();
    if (verbose_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1);
        std::cout << duration.count() / 1000.0 << " sec : Cutting" << std::endl;
    }
    
    // Step 3: Compute windowed alignments
    std::vector<AlignmentVector> note_alignments;
    
    for (size_t window_id = 0; window_id < score_note_arrays.size(); ++window_id) {
        if (alignment_type_ == "greedy") {
            auto alignment = (*greedy_symbolic_note_matcher_)(
                score_note_arrays[window_id], performance_note_arrays[window_id]
            );
            note_alignments.push_back(std::move(alignment));
        } else {
            TimeAlignmentVector dtw_alignment_times;
            
            if (alignment_type_ == "dtw") {
                if (score_note_arrays[window_id].empty() || performance_note_arrays[window_id].empty()) {
                    // For empty arrays, use linear interpolation from init times
                    if (window_id + 1 < dtw_alignment_times_init.size()) {
                        dtw_alignment_times = {
                            dtw_alignment_times_init[window_id],
                            dtw_alignment_times_init[window_id + 1]
                        };
                    }
                } else {
                    dtw_alignment_times = preprocessors::alignment_times_from_dtw(
                        score_note_arrays[window_id], performance_note_arrays[window_id],
                        *note_matcher_, score_fine_node_length_, s_time_div_, p_time_div_
                    );
                }
            } else {
                // Use linear alignment
                if (window_id + 1 < dtw_alignment_times_init.size()) {
                    dtw_alignment_times = {
                        dtw_alignment_times_init[window_id],
                        dtw_alignment_times_init[window_id + 1]
                    };
                }
            }
            
            // Distance augmented greedy alignment
            auto fine_alignment = (*symbolic_note_matcher_)(
                score_note_arrays[window_id], performance_note_arrays[window_id],
                dtw_alignment_times, shift_onsets_, cap_combinations_
            );
            
            note_alignments.push_back(std::move(fine_alignment));
        }
    }
    
    auto t3 = std::chrono::high_resolution_clock::now();
    if (verbose_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t3 - t2);
        std::cout << duration.count() / 1000.0 
                  << " sec : Fine-grained DTW passes, symbolic matching" << std::endl;
    }
    
    // Step 4: Mend windows to global alignment
    auto global_alignment = preprocessors::mend_note_alignments(
        note_alignments, performance_notes, score_notes, dtw_alignment_times_init
    );
    
    auto t4 = std::chrono::high_resolution_clock::now();
    if (verbose_time) {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3);
        std::cout << duration.count() / 1000.0 << " sec : Mending" << std::endl;
    }
    
    return global_alignment;
}

// Evaluation functions
namespace evaluation {

FScoreResult fscore_alignments(
    const AlignmentVector& prediction,
    const AlignmentVector& ground_truth,
    const std::vector<Alignment::Label>& types,
    bool return_numbers) {
    
    // Filter by types
    AlignmentVector pred_filtered, gt_filtered;
    
    for (const auto& align : prediction) {
        if (std::find(types.begin(), types.end(), align.label) != types.end()) {
            pred_filtered.push_back(align);
        }
    }
    
    for (const auto& align : ground_truth) {
        if (std::find(types.begin(), types.end(), align.label) != types.end()) {
            gt_filtered.push_back(align);
        }
    }
    
    // Count correct predictions
    size_t n_correct = 0;
    for (const auto& pred : pred_filtered) {
        for (const auto& gt : gt_filtered) {
            if (pred.label == gt.label &&
                pred.score_id == gt.score_id &&
                pred.performance_id == gt.performance_id) {
                n_correct++;
                break;
            }
        }
    }
    
    size_t n_pred_filtered = pred_filtered.size();
    size_t n_gt_filtered = gt_filtered.size();
    
    FScoreResult result;
    result.n_predicted = n_pred_filtered;
    result.n_ground_truth = n_gt_filtered;
    
    if (n_pred_filtered > 0 || n_gt_filtered > 0) {
        result.precision = n_pred_filtered > 0 ? static_cast<double>(n_correct) / n_pred_filtered : 0.0;
        result.recall = n_gt_filtered > 0 ? static_cast<double>(n_correct) / n_gt_filtered : 0.0;
        result.f_score = (result.precision + result.recall) > 0 ? 
                        2.0 * result.precision * result.recall / (result.precision + result.recall) : 0.0;
    } else {
        // No predictions and no ground truth -> perfect
        result.precision = result.recall = result.f_score = 1.0;
    }
    
    return result;
}

FScoreResult fscore_matches(
    const AlignmentVector& prediction,
    const AlignmentVector& ground_truth) {
    
    return fscore_alignments(prediction, ground_truth, {Alignment::Label::MATCH});
}

} // namespace evaluation

} // namespace parangonar