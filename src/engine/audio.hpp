#pragma once

#include <SDL.h>
#include <vector>
#include <string>
#include <cstdint>
#include "dr_mp3.h"

namespace ohpsp::engine {

enum class SoundEffect {
    CursorMove,
    TilePlace,     // General placement
    TilePlaceA,    // Crisp high droplet (Blue / Dot)
    TilePlaceB,    // Warm wooden knock (Red / Wall)
    TileClear,     // Soft descending sweep
    ActionBlocked, // Gentle error thud (fixed cell / invalid)
    HintRequested, // Sparkle arpeggio
    NumberSatisfied,// Crystalline bell chime
    PuzzleSolved,  // Grand triumphant chord
    MenuSelect,    // Bright confirmation ding
    MenuBack,      // Soft cancel drop
    VolumeToggle   // Crisp blip
};

struct TrackInfo {
    std::string title;
    std::string composer;
    std::string license;
    std::string filename;
};

class AudioManager {
public:
    static AudioManager& instance();

    bool init();
    void shutdown();

    void set_base_path(const std::string& path) { base_path_ = path; }

    void play_sfx(SoundEffect sfx);
    void play_music(bool play);
    void next_track();
    void prev_track();
    void set_track(int index);

    bool is_initialized() const { return initialized_; }
    bool is_music_enabled() const { return music_enabled_; }
    bool is_music_loaded() const { return mp3_loaded_; }
    void toggle_music() { play_music(!music_enabled_); }

    int current_track_index() const { return current_track_idx_; }
    const std::vector<TrackInfo>& get_tracks() const { return track_list_; }
    const TrackInfo& current_track() const { return track_list_[current_track_idx_]; }

private:
    AudioManager();
    ~AudioManager();

    void audio_callback(uint8_t* stream, int len);
    static void sdl_audio_callback(void* userdata, uint8_t* stream, int len);

    void lock();
    void unlock();

    SDL_AudioDeviceID audio_device_ = 0;
    bool initialized_ = false;
    bool music_enabled_ = true;
    int sample_rate_ = 44100;
    std::string base_path_;

    // dr_mp3 streaming
    drmp3 mp3_decoder_{};
    bool mp3_loaded_ = false;

    bool open_mp3_track(int index);
    void close_mp3_track();

    // Multi-voice procedural synthesizer
    struct SfxVoice {
        float frequency = 0.0f;
        float freq_slide = 0.0f;
        float phase = 0.0f;
        float amplitude = 0.0f;
        float decay = 0.05f;
        float harmonic2 = 0.0f;
        bool active = false;
    };
    static constexpr int NUM_VOICES = 8;
    SfxVoice sfx_voices_[NUM_VOICES]{};

    void trigger_voice(float freq, float freq_slide, float amp, float decay, float harmonic2 = 0.0f);

    std::vector<TrackInfo> track_list_;
    int current_track_idx_ = 0;

    std::string resolve_path(const std::string& filename);
};

} // namespace ohpsp::engine
