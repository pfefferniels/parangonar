#pragma once

#include <parangonar/note.hpp>
#include <parangonar/dtw.hpp>
#include <vector>
#include <utility>

namespace parangonar {

/**
 * Time alignment pair
 */
struct TimeAlignment {
    float score_time;
    float performance_time;
    
    TimeAlignment(float st, float pt) : score_time(st), performance_time(pt) {}
};

using TimeAlignmentVector = std::vector<TimeAlignment>;

/**
 * Preprocessing functions for note alignment
 */
namespace preprocessors {

/**
 * Compute alignment times from DTW on piano roll representations
 */
TimeAlignmentVector alignment_times_from_dtw(
    const NoteArray& score_notes,
    const NoteArray& performance_notes,
    const DynamicTimeWarping& matcher = DynamicTimeWarping(),
    float score_fine_node_length = 1.0f,
    int s_time_div = 16,
    int p_time_div = 16
);

/**
 * Cut note arrays into windows based on alignment times
 */
std::pair<std::vector<NoteArray>, std::vector<NoteArray>> cut_note_arrays(
    const NoteArray& performance_notes,
    const NoteArray& score_notes,
    const TimeAlignmentVector& alignment_times,
    float sfuzziness = 4.0f,
    float pfuzziness = 4.0f,
    int window_size = 1,
    bool pfuzziness_relative_to_tempo = true
);

/**
 * Mend windowed alignments into a global alignment
 */
AlignmentVector mend_note_alignments(
    const std::vector<AlignmentVector>& note_alignments,
    const NoteArray& performance_notes,
    const NoteArray& score_notes,
    const TimeAlignmentVector& node_times,
    int max_traversal_depth = 150
);

/**
 * Simple linear interpolation function
 */
class LinearInterpolator {
private:
    std::vector<float> x_vals;
    std::vector<float> y_vals;
    
public:
    LinearInterpolator(const std::vector<float>& x, const std::vector<float>& y);
    
    float interpolate(float x) const;
    std::vector<float> interpolate(const std::vector<float>& x_points) const;
};

} // namespace preprocessors
} // namespace parangonar