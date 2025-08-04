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
    
    // Default constructor
    Note() = default;
    
    // Static factory functions for clarity
    static Note score_note(float onset_beat, float duration_beat, int pitch, const std::string& id = "") {
        Note note;
        note.onset_beat = onset_beat;
        note.duration_beat = duration_beat;
        note.pitch = pitch;
        note.id = id;
        return note;
    }
    
    static Note performance_note(float onset_sec, float duration_sec, int pitch, int velocity, const std::string& id = "") {
        Note note;
        note.onset_sec = onset_sec;
        note.duration_sec = duration_sec;
        note.pitch = pitch;
        note.velocity = velocity;
        note.id = id;
        return note;
    }
};

// Type alias for note collections
using NoteArray = std::vector<Note>;

// Helper functions for note arrays
namespace note_array {

// Filter notes by pitch
NoteArray filter_by_pitch(const NoteArray& notes, int pitch);

// Get unique pitches
std::vector<int> unique_pitches(const NoteArray& notes);

// Get onset times (score or performance)
std::vector<float> onset_times_beat(const NoteArray& notes);
std::vector<float> onset_times_sec(const NoteArray& notes);

// Create piano roll representation
std::vector<std::vector<float>> compute_pianoroll(const NoteArray& notes, int time_div = 16, bool remove_drums = false);

} // namespace note_array

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