#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

namespace parangonar {

/**
 * Represents a musical note with timing and pitch information
 */
struct Note {
    // Score note fields
    float onset_beat = 0.0f;
    float duration_beat = 0.0f;
    float onset_quarter = 0.0f;
    float duration_quarter = 0.0f;
    int onset_div = 0;
    int duration_div = 0;
    
    // Performance note fields  
    float onset_sec = 0.0f;
    float duration_sec = 0.0f;
    int onset_tick = 0;
    int duration_tick = 0;
    int velocity = 0;
    int track = 0;
    int channel = 0;
    
    // Common fields
    int pitch = 0;
    int voice = 0;
    std::string id;
    int divs_pq = 16;
    
    // Constructor for score notes
    Note(float onset_beat, float duration_beat, int pitch, const std::string& id)
        : onset_beat(onset_beat), duration_beat(duration_beat), pitch(pitch), id(id) {}
    
    // Constructor for performance notes
    Note(float onset_sec, float duration_sec, int pitch, int velocity, const std::string& id)
        : onset_sec(onset_sec), duration_sec(duration_sec), pitch(pitch), velocity(velocity), id(id) {}
    
    // Default constructor
    Note() = default;
};

/**
 * Collection of notes with utility methods
 */
class NoteArray {
public:
    std::vector<Note> notes;
    
    NoteArray() = default;
    NoteArray(const std::vector<Note>& notes) : notes(notes) {}
    
    size_t size() const { return notes.size(); }
    bool empty() const { return notes.empty(); }
    
    Note& operator[](size_t idx) { return notes[idx]; }
    const Note& operator[](size_t idx) const { return notes[idx]; }
    
    auto begin() { return notes.begin(); }
    auto end() { return notes.end(); }
    auto begin() const { return notes.begin(); }
    auto end() const { return notes.end(); }
    
    // Filter notes by pitch
    NoteArray filter_by_pitch(int pitch) const;
    
    // Get unique pitches
    std::vector<int> unique_pitches() const;
    
    // Get onset times (score or performance)
    std::vector<float> onset_times_beat() const;
    std::vector<float> onset_times_sec() const;
    
    // Create piano roll representation
    std::vector<std::vector<float>> compute_pianoroll(int time_div = 16, bool remove_drums = false) const;
};

/**
 * Represents an alignment between score and performance notes
 */
struct Alignment {
    enum class Label {
        MATCH,
        INSERTION, 
        DELETION
    };
    
    Label label;
    std::string score_id;
    std::string performance_id;
    
    Alignment(Label label, const std::string& score_id = "", const std::string& performance_id = "")
        : label(label), score_id(score_id), performance_id(performance_id) {}
};

using AlignmentVector = std::vector<Alignment>;

} // namespace parangonar