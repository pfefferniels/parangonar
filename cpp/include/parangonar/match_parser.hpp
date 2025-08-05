#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <regex>
#include <parangonar/note.hpp>

namespace parangonar {

/**
 * Represents a score note from a match file
 */
struct MatchScoreNote {
    std::string id;
    std::string note_name;
    std::string accidental;
    int octave;
    std::string measure;
    int beat;
    float offset;
    float duration;
    float onset_time;
    float offset_time;
    std::vector<std::string> attributes;
};

/**
 * Represents a performance note from a match file
 */
struct MatchPerformanceNote {
    std::string id;
    std::string note_name;
    std::string accidental;
    int octave;
    int onset_tick;
    int offset_tick;
    int sound_off_tick;
    int velocity;
};

/**
 * Represents a match line (alignment between score and performance notes)
 */
struct MatchLine {
    enum Type {
        MATCH,      // snote-note pair
        DELETION,   // snote only
        INSERTION,  // note only  
        ORNAMENT    // ornament-note pair
    };
    
    Type type;
    MatchScoreNote score_note;
    MatchPerformanceNote performance_note;
    bool has_score_note = false;
    bool has_performance_note = false;
};

/**
 * Match file metadata
 */
struct MatchFileInfo {
    float version = 5.0f;
    int midi_clock_units = 480;
    int midi_clock_rate = 500000;
    std::string key_signature;
    std::string time_signature;
};

/**
 * Parsed match file data
 */
struct MatchFileData {
    MatchFileInfo info;
    std::vector<MatchLine> matches;
    std::vector<std::pair<int, int>> sustain_pedal; // time, value pairs
};

/**
 * Match file parser class
 */
class MatchFileParser {
public:
    /**
     * Parse a match file and return the parsed data
     */
    static MatchFileData parse_file(const std::string& filename);
    
    /**
     * Parse a single line from a match file
     */
    static MatchLine parse_match_line(const std::string& line);
    
    /**
     * Parse score note from string like "snote(n9,[C,n],3,1:1,0,1/4,0.0,1.0,[])"
     */
    static MatchScoreNote parse_score_note(const std::string& snote_str);
    
    /**
     * Parse performance note from string like "note(n0,[C,n],3,683,747,747,70)"
     */
    static MatchPerformanceNote parse_performance_note(const std::string& note_str);
    
    /**
     * Convert MatchFileData to Note arrays for score and performance
     */
    static std::pair<NoteArray, NoteArray> to_note_arrays(const MatchFileData& data);
    
    /**
     * Convert MatchFileData to alignment vector
     */
    static AlignmentVector to_alignment(const MatchFileData& data);
    
private:
    /**
     * Parse fractional duration like "1/4" or "3/16"
     */
    static float parse_fraction(const std::string& fraction_str);
    
    /**
     * Parse measure:beat like "1:1" or "2:2"
     */
    static std::pair<int, int> parse_measure_beat(const std::string& measure_beat_str);
    
    /**
     * Parse attribute list like "[staccato,grace]"
     */
    static std::vector<std::string> parse_attributes(const std::string& attr_str);
    
    /**
     * Convert note name and accidental to MIDI pitch
     */
    static int note_to_midi_pitch(const std::string& note_name, const std::string& accidental, int octave);
};

} // namespace parangonar