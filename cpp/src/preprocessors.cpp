#include <parangonar/preprocessors.hpp>
#include <algorithm>
#include <set>
#include <cmath>
#include <stdexcept>
#include <numeric>

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
    auto s_pianoroll = score_notes.compute_pianoroll(s_time_div, false);
    auto p_pianoroll = performance_notes.compute_pianoroll(p_time_div, false);
    
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
        for (const auto& note : score_notes.notes) {
            if (note.onset_beat >= window_start_score - score_margin &&
                note.onset_beat <= window_end_score + score_margin) {
                window_score_notes.notes.push_back(note);
            }
        }
        
        // Filter performance notes
        NoteArray window_perf_notes;
        for (const auto& note : performance_notes.notes) {
            if (note.onset_sec >= window_start_perf - perf_margin &&
                note.onset_sec <= window_end_perf + perf_margin) {
                window_perf_notes.notes.push_back(note);
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
    
    // Simple strategy: concatenate all alignments and remove duplicates
    std::set<std::string> used_score_ids;
    std::set<std::string> used_perf_ids;
    
    for (const auto& window_alignment : note_alignments) {
        for (const auto& align : window_alignment) {
            bool is_duplicate = false;
            
            if (align.label == Alignment::Label::MATCH) {
                if (used_score_ids.count(align.score_id) || 
                    used_perf_ids.count(align.performance_id)) {
                    is_duplicate = true;
                }
            } else if (align.label == Alignment::Label::DELETION) {
                if (used_score_ids.count(align.score_id)) {
                    is_duplicate = true;
                }
            } else if (align.label == Alignment::Label::INSERTION) {
                if (used_perf_ids.count(align.performance_id)) {
                    is_duplicate = true;
                }
            }
            
            if (!is_duplicate) {
                global_alignment.push_back(align);
                
                if (align.label == Alignment::Label::MATCH) {
                    used_score_ids.insert(align.score_id);
                    used_perf_ids.insert(align.performance_id);
                } else if (align.label == Alignment::Label::DELETION) {
                    used_score_ids.insert(align.score_id);
                } else if (align.label == Alignment::Label::INSERTION) {
                    used_perf_ids.insert(align.performance_id);
                }
            }
        }
    }
    
    // Add any unaligned notes as insertions or deletions
    for (const auto& note : score_notes.notes) {
        if (used_score_ids.find(note.id) == used_score_ids.end()) {
            global_alignment.emplace_back(Alignment::Label::DELETION, note.id);
        }
    }
    
    for (const auto& note : performance_notes.notes) {
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