#include <parangonar/match_parser.hpp>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cctype>

namespace parangonar {

MatchFileData MatchFileParser::parse_file(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open match file: " + filename);
    }
    
    MatchFileData data;
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;
        
        // Remove trailing period if present
        if (!line.empty() && line.back() == '.') {
            line.pop_back();
        }
        
        // Parse info lines
        if (line.find("info(") == 0) {
            if (line.find("matchFileVersion") != std::string::npos) {
                // Extract version number
                auto pos = line.find(',');
                if (pos != std::string::npos) {
                    std::string version_str = line.substr(pos + 1);
                    version_str.erase(std::remove(version_str.begin(), version_str.end(), ')'), version_str.end());
                    data.info.version = std::stof(version_str);
                }
            } else if (line.find("midiClockUnits") != std::string::npos) {
                auto pos = line.find(',');
                if (pos != std::string::npos) {
                    std::string units_str = line.substr(pos + 1);
                    units_str.erase(std::remove(units_str.begin(), units_str.end(), ')'), units_str.end());
                    data.info.midi_clock_units = std::stoi(units_str);
                }
            } else if (line.find("midiClockRate") != std::string::npos) {
                auto pos = line.find(',');
                if (pos != std::string::npos) {
                    std::string rate_str = line.substr(pos + 1);
                    rate_str.erase(std::remove(rate_str.begin(), rate_str.end(), ')'), rate_str.end());
                    data.info.midi_clock_rate = std::stoi(rate_str);
                }
            } else if (line.find("keySignature") != std::string::npos) {
                auto pos = line.find('[');
                auto end_pos = line.find(']');
                if (pos != std::string::npos && end_pos != std::string::npos) {
                    data.info.key_signature = line.substr(pos + 1, end_pos - pos - 1);
                }
            } else if (line.find("timeSignature") != std::string::npos) {
                auto pos = line.find('[');
                auto end_pos = line.find(']');
                if (pos != std::string::npos && end_pos != std::string::npos) {
                    data.info.time_signature = line.substr(pos + 1, end_pos - pos - 1);
                }
            }
        }
        // Parse sustain pedal lines
        else if (line.find("sustain(") == 0) {
            std::regex sustain_regex(R"(sustain\((\d+),(\d+)\))");
            std::smatch match;
            if (std::regex_search(line, match, sustain_regex)) {
                int time = std::stoi(match[1].str());
                int value = std::stoi(match[2].str());
                data.sustain_pedal.emplace_back(time, value);
            }
        }
        // Parse match lines (snote, insertion, etc.)
        else if (line.find("snote(") != std::string::npos || 
                 line.find("insertion-note(") != std::string::npos) {
            try {
                auto match_line = parse_match_line(line);
                data.matches.push_back(match_line);
            } catch (const std::exception& e) {
                std::cerr << "Warning: Failed to parse line: " << line << " - " << e.what() << std::endl;
            }
        }
        // Skip meta lines and other metadata for now
    }
    
    return data;
}

MatchLine MatchFileParser::parse_match_line(const std::string& line) {
    MatchLine match_line;
    
    // Check if it's an insertion
    if (line.find("insertion-note(") == 0) {
        match_line.type = MatchLine::INSERTION;
        match_line.has_score_note = false;
        match_line.has_performance_note = true;
        
        // Parse the performance note part
        auto note_start = line.find("insertion-note(");
        if (note_start != std::string::npos) {
            auto note_end = line.find(')', note_start + 15);
            if (note_end != std::string::npos) {
                std::string note_str = line.substr(note_start, note_end - note_start + 1);
                match_line.performance_note = parse_performance_note(note_str);
            }
        }
    }
    // Check if it's a match (snote-note pair)
    else if (line.find("snote(") != std::string::npos && line.find(")-note(") != std::string::npos) {
        match_line.type = MatchLine::MATCH;
        match_line.has_score_note = true;
        match_line.has_performance_note = true;
        
        // Split on ")-note("
        auto split_pos = line.find(")-note(");
        if (split_pos != std::string::npos) {
            std::string snote_part = line.substr(0, split_pos + 1);
            std::string note_part = "note(" + line.substr(split_pos + 7);
            
            match_line.score_note = parse_score_note(snote_part);
            match_line.performance_note = parse_performance_note(note_part);
        }
    }
    // Check if it's a deletion (snote only)
    else if (line.find("snote(") != std::string::npos) {
        match_line.type = MatchLine::DELETION;
        match_line.has_score_note = true;
        match_line.has_performance_note = false;
        
        // Find the full snote(...) part
        auto snote_start = line.find("snote(");
        auto snote_end = line.find(')', snote_start);
        if (snote_end != std::string::npos) {
            std::string snote_str = line.substr(snote_start, snote_end - snote_start + 1);
            match_line.score_note = parse_score_note(snote_str);
        }
    }
    
    return match_line;
}

MatchScoreNote MatchFileParser::parse_score_note(const std::string& snote_str) {
    MatchScoreNote score_note;
    
    // Extract content between snote( and )
    auto start = snote_str.find("snote(");
    auto end = snote_str.rfind(')');
    if (start == std::string::npos || end == std::string::npos) {
        throw std::runtime_error("Invalid snote format: " + snote_str);
    }
    
    std::string content = snote_str.substr(start + 6, end - start - 6);
    
    // Split by commas, but be careful of commas inside brackets
    std::vector<std::string> parts;
    std::string current_part;
    int bracket_depth = 0;
    
    for (char c : content) {
        if (c == '[') bracket_depth++;
        else if (c == ']') bracket_depth--;
        else if (c == ',' && bracket_depth == 0) {
            parts.push_back(current_part);
            current_part.clear();
            continue;
        }
        current_part += c;
    }
    if (!current_part.empty()) {
        parts.push_back(current_part);
    }
    
    if (parts.size() < 8) {
        throw std::runtime_error("Invalid snote format, not enough parts: " + snote_str);
    }
    
    // Parse each part
    score_note.id = parts[0];
    
    // Parse note and accidental from [C,n] format
    std::string note_part = parts[1];
    if (note_part.front() == '[' && note_part.back() == ']') {
        note_part = note_part.substr(1, note_part.length() - 2);
        auto comma_pos = note_part.find(',');
        if (comma_pos != std::string::npos) {
            score_note.note_name = note_part.substr(0, comma_pos);
            score_note.accidental = note_part.substr(comma_pos + 1);
        }
    }
    
    score_note.octave = std::stoi(parts[2]);
    
    // Parse measure:beat
    auto measure_beat = parse_measure_beat(parts[3]);
    score_note.measure = std::to_string(measure_beat.first) + ":" + std::to_string(measure_beat.second);
    score_note.beat = measure_beat.second;
    
    score_note.offset = parse_fraction(parts[4]);
    score_note.duration = parse_fraction(parts[5]);
    score_note.onset_time = std::stof(parts[6]);
    score_note.offset_time = std::stof(parts[7]);
    
    // Parse attributes if present
    if (parts.size() > 8) {
        score_note.attributes = parse_attributes(parts[8]);
    }
    
    return score_note;
}

MatchPerformanceNote MatchFileParser::parse_performance_note(const std::string& note_str) {
    MatchPerformanceNote perf_note;
    
    // Extract content between note( and ) or insertion-note( and )
    size_t start_pos = 0;
    if (note_str.find("insertion-note(") == 0) {
        start_pos = 15; // length of "insertion-note("
    } else if (note_str.find("note(") == 0) {
        start_pos = 5; // length of "note("
    } else {
        throw std::runtime_error("Invalid note format: " + note_str);
    }
    
    auto end = note_str.rfind(')');
    if (end == std::string::npos) {
        throw std::runtime_error("Invalid note format: " + note_str);
    }
    
    std::string content = note_str.substr(start_pos, end - start_pos);
    
    // Split by commas, but be careful of commas inside brackets
    std::vector<std::string> parts;
    std::string current_part;
    int bracket_depth = 0;
    
    for (char c : content) {
        if (c == '[') bracket_depth++;
        else if (c == ']') bracket_depth--;
        else if (c == ',' && bracket_depth == 0) {
            parts.push_back(current_part);
            current_part.clear();
            continue;
        }
        current_part += c;
    }
    if (!current_part.empty()) {
        parts.push_back(current_part);
    }
    
    if (parts.size() < 7) {
        throw std::runtime_error("Invalid note format, not enough parts: " + note_str);
    }
    
    // Parse each part
    perf_note.id = parts[0];
    
    // Parse note and accidental from [C,n] format
    std::string note_part = parts[1];
    if (note_part.front() == '[' && note_part.back() == ']') {
        note_part = note_part.substr(1, note_part.length() - 2);
        auto comma_pos = note_part.find(',');
        if (comma_pos != std::string::npos) {
            perf_note.note_name = note_part.substr(0, comma_pos);
            perf_note.accidental = note_part.substr(comma_pos + 1);
        }
    }
    
    perf_note.octave = std::stoi(parts[2]);
    perf_note.onset_tick = std::stoi(parts[3]);
    perf_note.offset_tick = std::stoi(parts[4]);
    perf_note.sound_off_tick = std::stoi(parts[5]);
    perf_note.velocity = std::stoi(parts[6]);
    
    return perf_note;
}

std::pair<NoteArray, NoteArray> MatchFileParser::to_note_arrays(const MatchFileData& data) {
    NoteArray score_notes, performance_notes;
    
    // Convert timing parameters
    float mpq = data.info.midi_clock_rate; // microseconds per quarter
    float ppq = data.info.midi_clock_units; // ticks per quarter
    
    for (const auto& match : data.matches) {
        // Add score note if present
        if (match.has_score_note) {
            Note score_note;
            score_note.id = match.score_note.id;
            score_note.onset_beat = match.score_note.onset_time;
            score_note.duration_beat = match.score_note.offset_time - match.score_note.onset_time;
            score_note.pitch = note_to_midi_pitch(match.score_note.note_name, 
                                                  match.score_note.accidental, 
                                                  match.score_note.octave);
            score_notes.push_back(score_note);
        }
        
        // Add performance note if present
        if (match.has_performance_note) {
            Note perf_note;
            perf_note.id = match.performance_note.id;
            perf_note.onset_tick = match.performance_note.onset_tick;
            perf_note.duration_tick = match.performance_note.offset_tick - match.performance_note.onset_tick;
            
            // Convert ticks to seconds: seconds = ticks * (microseconds_per_quarter / ticks_per_quarter) / 1000000
            perf_note.onset_sec = (match.performance_note.onset_tick * mpq / ppq) / 1000000.0f;
            perf_note.duration_sec = (perf_note.duration_tick * mpq / ppq) / 1000000.0f;
            
            perf_note.pitch = note_to_midi_pitch(match.performance_note.note_name,
                                                 match.performance_note.accidental,
                                                 match.performance_note.octave);
            perf_note.velocity = match.performance_note.velocity;
            performance_notes.push_back(perf_note);
        }
    }
    
    return {score_notes, performance_notes};
}

AlignmentVector MatchFileParser::to_alignment(const MatchFileData& data) {
    AlignmentVector alignment;
    
    for (const auto& match : data.matches) {
        switch (match.type) {
            case MatchLine::MATCH:
                alignment.emplace_back(Alignment::Label::MATCH, 
                                       match.score_note.id, 
                                       match.performance_note.id);
                break;
            case MatchLine::DELETION:
                alignment.emplace_back(Alignment::Label::DELETION, 
                                       match.score_note.id, 
                                       "");
                break;
            case MatchLine::INSERTION:
                alignment.emplace_back(Alignment::Label::INSERTION, 
                                       "", 
                                       match.performance_note.id);
                break;
            case MatchLine::ORNAMENT:
                // Treat ornaments as matches for now
                alignment.emplace_back(Alignment::Label::MATCH, 
                                       match.score_note.id, 
                                       match.performance_note.id);
                break;
        }
    }
    
    return alignment;
}

float MatchFileParser::parse_fraction(const std::string& fraction_str) {
    auto slash_pos = fraction_str.find('/');
    if (slash_pos == std::string::npos) {
        return std::stof(fraction_str);
    }
    
    float numerator = std::stof(fraction_str.substr(0, slash_pos));
    float denominator = std::stof(fraction_str.substr(slash_pos + 1));
    
    return numerator / denominator;
}

std::pair<int, int> MatchFileParser::parse_measure_beat(const std::string& measure_beat_str) {
    auto colon_pos = measure_beat_str.find(':');
    if (colon_pos == std::string::npos) {
        throw std::runtime_error("Invalid measure:beat format: " + measure_beat_str);
    }
    
    int measure = std::stoi(measure_beat_str.substr(0, colon_pos));
    int beat = std::stoi(measure_beat_str.substr(colon_pos + 1));
    
    return {measure, beat};
}

std::vector<std::string> MatchFileParser::parse_attributes(const std::string& attr_str) {
    std::vector<std::string> attributes;
    
    if (attr_str.empty() || attr_str == "[]") {
        return attributes;
    }
    
    std::string content = attr_str;
    if (content.front() == '[' && content.back() == ']') {
        content = content.substr(1, content.length() - 2);
    }
    
    std::stringstream ss(content);
    std::string item;
    while (std::getline(ss, item, ',')) {
        attributes.push_back(item);
    }
    
    return attributes;
}

int MatchFileParser::note_to_midi_pitch(const std::string& note_name, const std::string& accidental, int octave) {
    // Map note names to semitones from C
    std::unordered_map<std::string, int> note_map = {
        {"C", 0}, {"D", 2}, {"E", 4}, {"F", 5}, {"G", 7}, {"A", 9}, {"B", 11}
    };
    
    auto it = note_map.find(note_name);
    if (it == note_map.end()) {
        throw std::runtime_error("Unknown note name: " + note_name);
    }
    
    int semitone = it->second;
    
    // Apply accidental
    if (accidental == "#") {
        semitone += 1;
    } else if (accidental == "b") {
        semitone -= 1;
    }
    // "n" means natural, no change needed
    
    // Calculate MIDI pitch: C4 = 60
    int midi_pitch = (octave + 1) * 12 + semitone;
    
    return midi_pitch;
}

} // namespace parangonar