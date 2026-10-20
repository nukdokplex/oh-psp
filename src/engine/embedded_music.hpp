#pragma once
#include <cstddef>
#include <cstdint>

extern "C" {
    extern const uint8_t track1_mp3_start[];
    extern const uint8_t track1_mp3_end[];

    extern const uint8_t track2_mp3_start[];
    extern const uint8_t track2_mp3_end[];

    extern const uint8_t track3_mp3_start[];
    extern const uint8_t track3_mp3_end[];
}

namespace ohpsp::engine {

struct EmbeddedTrack {
    const uint8_t* data;
    size_t size;
};

inline EmbeddedTrack get_embedded_track(int index) {
    switch (index) {
        case 0: return {track1_mp3_start, static_cast<size_t>(track1_mp3_end - track1_mp3_start)};
        case 1: return {track2_mp3_start, static_cast<size_t>(track2_mp3_end - track2_mp3_start)};
        case 2: return {track3_mp3_start, static_cast<size_t>(track3_mp3_end - track3_mp3_start)};
        default: return {nullptr, 0};
    }
}

} // namespace ohpsp::engine
