#include <parangonar/preprocessors.hpp>
#include <parangonar/matchers.hpp>
#include <algorithm>
#include <set>
#include <map>
#include <cmath>
#include <stdexcept>
#include <numeric>
#include <limits>

namespace parangonar {
namespace preprocessors {

TimeAlignmentVector alignment_times_from_dtw(
    const NoteArray& score_notes,
    const NoteArray& performance_notes,
    const DynamicTimeWarping& matcher,
    float score_fine_node_length,
    int s_time_div,
    int p_time_div) {
    
    // Compute piano rolls
    auto s_pianoroll = note_array::compute_pianoroll(score_notes, s_time_div, false);
    auto p_pianoroll = note_array::compute_pianoroll(performance_notes, p_time_div, false);
    
    // Make performance piano roll binary
    for (auto& row : p_pianoroll) {
        for (auto& val : row) {
            val = (val > 0.0f) ? 1.0f : 0.0f;
        }
    }
    
    // Transpose piano rolls for DTW (time x pitch -> pitch x time)
    std::vector<std::vector<float>> s_pianoroll_T, p_pianoroll_T;
    
    if (!s_pianoroll.empty() && !s_pianoroll[0].empty()) {
        s_pianoroll_T.resize(s_pianoroll[0].size());
        for (size_t i = 0; i < s_pianoroll[0].size(); ++i) {
            s_pianoroll_T[i].resize(s_pianoroll.size());
            for (size_t j = 0; j < s_pianoroll.size(); ++j) {
                s_pianoroll_T[i][j] = s_pianoroll[j][i];
            }
        }
    }
    
    if (!p_pianoroll.empty() && !p_pianoroll[0].empty()) {
        p_pianoroll_T.resize(p_pianoroll[0].size());
        for (size_t i = 0; i < p_pianoroll[0].size(); ++i) {
            p_pianoroll_T[i].resize(p_pianoroll.size());
            for (size_t j = 0; j < p_pianoroll.size(); ++j) {
                p_pianoroll_T[i][j] = p_pianoroll[j][i];
            }
        }
    }
    
    // Perform DTW alignment
    auto dtw_result = matcher.compute(s_pianoroll_T, p_pianoroll_T, true, false);
    
    // Convert path to time alignments
    TimeAlignmentVector alignment_times;
    
    for (const auto& step : dtw_result.path) {
        float score_time = static_cast<float>(step.row) / s_time_div;
        float performance_time = static_cast<float>(step.col) / p_time_div;
        alignment_times.emplace_back(score_time, performance_time);
    }
    
    // Remove duplicates and sort
    std::sort(alignment_times.begin(), alignment_times.end(),
              [](const TimeAlignment& a, const TimeAlignment& b) {
                  return a.score_time < b.score_time;
              });
    
    // Remove consecutive duplicates
    alignment_times.erase(
        std::unique(alignment_times.begin(), alignment_times.end(),
                   [](const TimeAlignment& a, const TimeAlignment& b) {
                       return std::abs(a.score_time - b.score_time) < 1e-6f;
                   }),
        alignment_times.end()
    );
    
    return alignment_times;
}

std::pair<std::vector<NoteArray>, std::vector<NoteArray>> cut_note_arrays(
    const NoteArray& performance_notes,
    const NoteArray& score_notes,
    const TimeAlignmentVector& alignment_times,
    float sfuzziness,
    float pfuzziness,
    int window_size,
    bool pfuzziness_relative_to_tempo) {
    
    std::vector<NoteArray> score_arrays;
    std::vector<NoteArray> performance_arrays;
    
    if (alignment_times.size() < 2) {
        // Not enough alignment points, return original arrays
        score_arrays.push_back(score_notes);
        performance_arrays.push_back(performance_notes);
        return {score_arrays, performance_arrays};
    }
    
    // Create interpolator for score time to performance time mapping
    std::vector<float> score_times, perf_times;
    for (const auto& align : alignment_times) {
        score_times.push_back(align.score_time);
        perf_times.push_back(align.performance_time);
    }
    
    LinearInterpolator interpolator(score_times, perf_times);
    
    // Cut arrays into windows
    for (size_t i = 0; i < alignment_times.size() - window_size; ++i) {
        float window_start_score = alignment_times[i].score_time;
        float window_end_score = alignment_times[i + window_size].score_time;
        
        float window_start_perf = alignment_times[i].performance_time;
        float window_end_perf = alignment_times[i + window_size].performance_time;
        
        // Apply fuzziness
        float score_margin = sfuzziness;
        float perf_margin = pfuzziness;
        
        if (pfuzziness_relative_to_tempo && i + window_size < alignment_times.size()) {
            float tempo_ratio = (window_end_perf - window_start_perf) / 
                               std::max(window_end_score - window_start_score, 1e-6f);
            perf_margin = pfuzziness * tempo_ratio;
        }
        
        // Filter score notes
        NoteArray window_score_notes;
        for (const auto& note : score_notes) {
            if (note.onset_beat >= window_start_score - score_margin &&
                note.onset_beat <= window_end_score + score_margin) {
                window_score_notes.push_back(note);
            }
        }
        
        // Filter performance notes
        NoteArray window_perf_notes;
        for (const auto& note : performance_notes) {
            if (note.onset_sec >= window_start_perf - perf_margin &&
                note.onset_sec <= window_end_perf + perf_margin) {
                window_perf_notes.push_back(note);
            }
        }
        
        score_arrays.push_back(std::move(window_score_notes));
        performance_arrays.push_back(std::move(window_perf_notes));
    }
    
    return {score_arrays, performance_arrays};
}

AlignmentVector mend_note_alignments(
    const std::vector<AlignmentVector>& note_alignments,
    const NoteArray& performance_notes,
    const NoteArray& score_notes,
    const TimeAlignmentVector& node_times,
    int max_traversal_depth) {
    
    AlignmentVector global_alignment;
    
    // Collect all matches, keeping track of conflicts
    std::map<std::string, std::vector<std::pair<int, std::string>>> score_to_perf_candidates; // score_id -> [(window_id, perf_id)]
    std::map<std::string, std::vector<std::pair<int, std::string>>> perf_to_score_candidates; // perf_id -> [(window_id, score_id)]
    std::set<std::string> deleted_scores;
    std::set<std::string> inserted_perfs;
    
    // Collect all potential matches and conflicts
    for (size_t window_id = 0; window_id < note_alignments.size(); ++window_id) {
        for (const auto& align : note_alignments[window_id]) {
            if (align.label == Alignment::Label::MATCH) {
                score_to_perf_candidates[align.score_id].emplace_back(window_id, align.performance_id);
                perf_to_score_candidates[align.performance_id].emplace_back(window_id, align.score_id);
            } else if (align.label == Alignment::Label::DELETION) {
                deleted_scores.insert(align.score_id);
            } else if (align.label == Alignment::Label::INSERTION) {
                inserted_perfs.insert(align.performance_id);
            }
        }
    }
    
    std::set<std::string> used_score_ids;
    std::set<std::string> used_perf_ids;
    
    // Resolve matches, preferring earlier windows for conflicts
    for (const auto& [score_id, candidates] : score_to_perf_candidates) {
        if (candidates.size() == 1) {
            // Unique match - easy case
            const auto& [window_id, perf_id] = candidates[0];
            if (perf_to_score_candidates[perf_id].size() == 1) {
                // Mutual unique match - accept it
                global_alignment.emplace_back(Alignment::Label::MATCH, score_id, perf_id);
                used_score_ids.insert(score_id);
                used_perf_ids.insert(perf_id);
            } else {
                // Performance note has multiple score candidates - need to resolve
                // Choose the candidate from the earliest window
                const auto& perf_candidates = perf_to_score_candidates[perf_id];
                int best_window = std::numeric_limits<int>::max();
                std::string best_score;
                for (const auto& [win_id, cand_score_id] : perf_candidates) {
                    if (win_id < best_window && used_score_ids.find(cand_score_id) == used_score_ids.end()) {
                        best_window = win_id;
                        best_score = cand_score_id;
                    }
                }
                if (!best_score.empty() && used_perf_ids.find(perf_id) == used_perf_ids.end()) {
                    global_alignment.emplace_back(Alignment::Label::MATCH, best_score, perf_id);
                    used_score_ids.insert(best_score);
                    used_perf_ids.insert(perf_id);
                }
            }
        } else {
            // Score note has multiple performance candidates - choose the best one
            // Find the candidate from the earliest window with an available performance note
            int best_window = std::numeric_limits<int>::max();
            std::string best_perf;
            for (const auto& [window_id, perf_id] : candidates) {
                if (window_id < best_window && used_perf_ids.find(perf_id) == used_perf_ids.end()) {
                    // Also check if this performance note doesn't have a better score candidate
                    bool is_best_for_perf = true;
                    const auto& perf_candidates = perf_to_score_candidates[perf_id];
                    for (const auto& [other_win_id, other_score_id] : perf_candidates) {
                        if (other_win_id < window_id && used_score_ids.find(other_score_id) == used_score_ids.end()) {
                            is_best_for_perf = false;
                            break;
                        }
                    }
                    if (is_best_for_perf) {
                        best_window = window_id;
                        best_perf = perf_id;
                    }
                }
            }
            if (!best_perf.empty() && used_score_ids.find(score_id) == used_score_ids.end()) {
                global_alignment.emplace_back(Alignment::Label::MATCH, score_id, best_perf);
                used_score_ids.insert(score_id);
                used_perf_ids.insert(best_perf);
            }
        }
    }
    
    // Add greedy fallback for unmatched notes with similar pitches nearby
    SimplestGreedyMatcher greedy_fallback;
    
    // Create arrays of unmatched notes
    NoteArray unmatched_score_notes, unmatched_perf_notes;
    for (const auto& note : score_notes) {
        if (used_score_ids.find(note.id) == used_score_ids.end()) {
            unmatched_score_notes.push_back(note);
        }
    }
    for (const auto& note : performance_notes) {
        if (used_perf_ids.find(note.id) == used_perf_ids.end()) {
            unmatched_perf_notes.push_back(note);
        }
    }
    
    // Try greedy matching on remaining notes
    if (!unmatched_score_notes.empty() && !unmatched_perf_notes.empty()) {
        auto fallback_alignment = greedy_fallback(unmatched_score_notes, unmatched_perf_notes);
        for (const auto& align : fallback_alignment) {
            if (align.label == Alignment::Label::MATCH) {
                // Only add if both notes are still unmatched
                if (used_score_ids.find(align.score_id) == used_score_ids.end() &&
                    used_perf_ids.find(align.performance_id) == used_perf_ids.end()) {
                    global_alignment.push_back(align);
                    used_score_ids.insert(align.score_id);
                    used_perf_ids.insert(align.performance_id);
                }
            }
        }
    }
    
    // Add final deletions and insertions for truly unmatched notes
    for (const auto& note : score_notes) {
        if (used_score_ids.find(note.id) == used_score_ids.end()) {
            global_alignment.emplace_back(Alignment::Label::DELETION, note.id);
        }
    }
    
    for (const auto& note : performance_notes) {
        if (used_perf_ids.find(note.id) == used_perf_ids.end()) {
            global_alignment.emplace_back(Alignment::Label::INSERTION, "", note.id);
        }
    }
    
    return global_alignment;
}

// LinearInterpolator implementation
LinearInterpolator::LinearInterpolator(const std::vector<float>& x, const std::vector<float>& y)
    : x_vals(x), y_vals(y) {
    if (x.size() != y.size() || x.empty()) {
        throw std::invalid_argument("x and y must have the same non-zero size");
    }
    
    // Ensure x values are sorted
    std::vector<size_t> indices(x.size());
    std::iota(indices.begin(), indices.end(), 0);
    std::sort(indices.begin(), indices.end(),
              [&x](size_t i, size_t j) { return x[i] < x[j]; });
    
    std::vector<float> sorted_x(x.size()), sorted_y(x.size());
    for (size_t i = 0; i < indices.size(); ++i) {
        sorted_x[i] = x[indices[i]];
        sorted_y[i] = y[indices[i]];
    }
    
    x_vals = std::move(sorted_x);
    y_vals = std::move(sorted_y);
}

float LinearInterpolator::interpolate(float x) const {
    if (x_vals.size() == 1) {
        return y_vals[0];
    }
    
    // Extrapolation
    if (x <= x_vals[0]) {
        return y_vals[0];
    }
    if (x >= x_vals.back()) {
        return y_vals.back();
    }
    
    // Find surrounding points
    auto it = std::lower_bound(x_vals.begin(), x_vals.end(), x);
    size_t idx = std::distance(x_vals.begin(), it);
    
    if (idx == 0) {
        return y_vals[0];
    }
    
    size_t i0 = idx - 1;
    size_t i1 = idx;
    
    float x0 = x_vals[i0];
    float x1 = x_vals[i1];
    float y0 = y_vals[i0];
    float y1 = y_vals[i1];
    
    // Linear interpolation
    float t = (x - x0) / (x1 - x0);
    return y0 + t * (y1 - y0);
}

std::vector<float> LinearInterpolator::interpolate(const std::vector<float>& x_points) const {
    std::vector<float> result;
    result.reserve(x_points.size());
    
    for (float x : x_points) {
        result.push_back(interpolate(x));
    }
    
    return result;
}

} // namespace preprocessors
} // namespace parangonar