#include <parangonar/note.hpp>
#include <algorithm>
#include <set>
#include <cmath>

namespace parangonar {
namespace note_array {

NoteArray filter_by_pitch(const NoteArray& notes, int pitch) {
    NoteArray result;
    for (const auto& note : notes) {
        if (note.pitch == pitch) {
            result.push_back(note);
        }
    }
    return result;
}

std::vector<int> unique_pitches(const NoteArray& notes) {
    std::set<int> pitch_set;
    for (const auto& note : notes) {
        pitch_set.insert(note.pitch);
    }
    return std::vector<int>(pitch_set.begin(), pitch_set.end());
}

std::vector<float> onset_times_beat(const NoteArray& notes) {
    std::vector<float> times;
    times.reserve(notes.size());
    for (const auto& note : notes) {
        times.push_back(note.onset_beat);
    }
    return times;
}

std::vector<float> onset_times_sec(const NoteArray& notes) {
    std::vector<float> times;
    times.reserve(notes.size());
    for (const auto& note : notes) {
        times.push_back(note.onset_sec);
    }
    return times;
}

std::vector<std::vector<float>> compute_pianoroll(const NoteArray& notes, int time_div, bool remove_drums) {
    if (notes.empty()) {
        return {};
    }
    
    // Find time range and pitch range
    float min_time = 0.0f;
    float max_time = 0.0f;
    int min_pitch = 127;
    int max_pitch = 0;
    
    bool use_beat_time = (notes[0].onset_beat != 0.0f || notes[0].duration_beat != 0.0f);
    
    for (const auto& note : notes) {
        if (remove_drums && note.pitch >= 128) continue;
        
        float onset = use_beat_time ? note.onset_beat : note.onset_sec;
        float duration = use_beat_time ? note.duration_beat : note.duration_sec;
        
        min_time = std::min(min_time, onset);
        max_time = std::max(max_time, onset + duration);
        min_pitch = std::min(min_pitch, note.pitch);
        max_pitch = std::max(max_pitch, note.pitch);
    }
    
    // Create piano roll matrix
    int num_time_steps = static_cast<int>(std::ceil(max_time * time_div)) + 1;
    int num_pitches = max_pitch - min_pitch + 1;
    
    std::vector<std::vector<float>> pianoroll(num_time_steps, std::vector<float>(num_pitches, 0.0f));
    
    // Fill piano roll
    for (const auto& note : notes) {
        if (remove_drums && note.pitch >= 128) continue;
        
        float onset = use_beat_time ? note.onset_beat : note.onset_sec;
        float duration = use_beat_time ? note.duration_beat : note.duration_sec;
        
        int start_time = static_cast<int>(onset * time_div);
        int end_time = static_cast<int>((onset + duration) * time_div);
        int pitch_idx = note.pitch - min_pitch;
        
        for (int t = start_time; t <= end_time && t < num_time_steps; ++t) {
            pianoroll[t][pitch_idx] = 1.0f;
        }
    }
    
    return pianoroll;
}

} // namespace note_array
} // namespace parangonar