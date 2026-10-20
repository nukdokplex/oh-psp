#define DR_MP3_IMPLEMENTATION
#include "audio.hpp"
#include "embedded_music.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <algorithm>

namespace ohpsp::engine {

AudioManager& AudioManager::instance() {
    static AudioManager mgr;
    return mgr;
}

AudioManager::AudioManager() {
    track_list_ = {
        {"Cozy Puzzle Title", "MintoDog", "CC0 / Public Domain", "track1_title.mp3"},
        {"Cozy Puzzle In-Game 1", "MintoDog", "CC0 / Public Domain", "track2_ingame1.mp3"},
        {"Cozy Puzzle In-Game 2", "MintoDog", "CC0 / Public Domain", "track3_ingame2.mp3"}
    };
    current_track_idx_ = 0;
}

AudioManager::~AudioManager() {
    shutdown();
}

std::string AudioManager::resolve_path(const std::string& filename) {
    std::vector<std::string> search_dirs;
    if (!base_path_.empty()) {
        search_dirs.push_back(base_path_ + "/assets/music/");
        search_dirs.push_back(base_path_ + "/assets/");
        search_dirs.push_back(base_path_ + "/music/");
        search_dirs.push_back(base_path_ + "/");
    }
    search_dirs.push_back("assets/music/");
    search_dirs.push_back("assets/");
    search_dirs.push_back("music/");
    search_dirs.push_back("");
    search_dirs.push_back("../assets/music/");
    search_dirs.push_back("../../assets/music/");
    search_dirs.push_back("ms0:/PSP/GAME/OHPSP/assets/music/");
    search_dirs.push_back("ms0:/PSP/GAME/OHPSP/music/");
    search_dirs.push_back("ms0:/PSP/GAME/OHPSP/");
    search_dirs.push_back("ms0:/PSP/GAME/oh-psp/assets/music/");
    search_dirs.push_back("ms0:/PSP/GAME/ohpsp/assets/music/");

    for (const auto& dir : search_dirs) {
        std::string candidate = dir + filename;
        FILE* f = fopen(candidate.c_str(), "rb");
        if (f) {
            fclose(f);
            return candidate;
        }
    }
    return "";
}

bool AudioManager::init() {
    if (initialized_) return true;

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            return false;
        }
    }

    SDL_AudioSpec wanted, have;
    SDL_zero(wanted);
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 1024;
    wanted.callback = sdl_audio_callback;
    wanted.userdata = this;

    audio_device_ = SDL_OpenAudioDevice(nullptr, 0, &wanted, &have, 0);
    if (audio_device_ == 0) {
        if (SDL_OpenAudio(&wanted, &have) == 0) {
            audio_device_ = 1;
        } else {
            return false;
        }
    }

    sample_rate_ = have.freq > 0 ? have.freq : 44100;

    // Attempt to open the first music track
    open_mp3_track(current_track_idx_);

    if (audio_device_ > 1) {
        SDL_PauseAudioDevice(audio_device_, 0);
    } else {
        SDL_PauseAudio(0);
    }

    initialized_ = true;
    return true;
}

void AudioManager::shutdown() {
    if (initialized_) {
        close_mp3_track();
        if (audio_device_ > 1) {
            SDL_PauseAudioDevice(audio_device_, 1);
            SDL_CloseAudioDevice(audio_device_);
        } else if (audio_device_ == 1) {
            SDL_PauseAudio(1);
            SDL_CloseAudio();
        }
        audio_device_ = 0;
        initialized_ = false;
    }
}

void AudioManager::lock() {
    if (!initialized_) return;
    if (audio_device_ > 1) {
        SDL_LockAudioDevice(audio_device_);
    } else if (audio_device_ == 1) {
        SDL_LockAudio();
    }
}

void AudioManager::unlock() {
    if (!initialized_) return;
    if (audio_device_ > 1) {
        SDL_UnlockAudioDevice(audio_device_);
    } else if (audio_device_ == 1) {
        SDL_UnlockAudio();
    }
}

void AudioManager::close_mp3_track() {
    if (mp3_loaded_) {
        lock();
        drmp3_uninit(&mp3_decoder_);
        mp3_loaded_ = false;
        unlock();
    }
}

bool AudioManager::open_mp3_track(int index) {
    if (index < 0 || index >= static_cast<int>(track_list_.size())) {
        return false;
    }

    close_mp3_track();

    lock();
    // 1. Try embedded track first (instant, self-contained, no disk access)
    auto embedded = get_embedded_track(index);
    if (embedded.data != nullptr && embedded.size > 0) {
        if (drmp3_init_memory(&mp3_decoder_, embedded.data, embedded.size, nullptr)) {
            mp3_loaded_ = true;
            current_track_idx_ = index;
            unlock();
            return true;
        }
    }

    // 2. Fallback to external file if embedded track failed
    std::string path = resolve_path(track_list_[index].filename);
    if (!path.empty()) {
        if (drmp3_init_file(&mp3_decoder_, path.c_str(), nullptr)) {
            mp3_loaded_ = true;
            current_track_idx_ = index;
        }
    }
    unlock();

    return mp3_loaded_;
}

void AudioManager::play_music(bool play) {
    music_enabled_ = play;
}

void AudioManager::set_track(int index) {
    open_mp3_track(index);
}

void AudioManager::next_track() {
    int next_idx = (current_track_idx_ + 1) % static_cast<int>(track_list_.size());
    open_mp3_track(next_idx);
}

void AudioManager::prev_track() {
    int prev_idx = (current_track_idx_ - 1 + static_cast<int>(track_list_.size())) % static_cast<int>(track_list_.size());
    open_mp3_track(prev_idx);
}

void AudioManager::trigger_voice(float freq, float freq_slide, float amp, float decay, float harmonic2) {
    int best_idx = 0;
    float min_amp = 999.0f;
    for (int i = 0; i < NUM_VOICES; ++i) {
        if (!sfx_voices_[i].active) {
            best_idx = i;
            break;
        }
        if (sfx_voices_[i].amplitude < min_amp) {
            min_amp = sfx_voices_[i].amplitude;
            best_idx = i;
        }
    }
    sfx_voices_[best_idx] = SfxVoice{freq, freq_slide, 0.0f, amp, decay, harmonic2, true};
}

void AudioManager::play_sfx(SoundEffect sfx) {
    if (!initialized_) return;

    lock();
    switch (sfx) {
        case SoundEffect::CursorMove:
            // Crisp, gentle woodblock UI tick
            trigger_voice(650.0f, -2500.0f, 0.14f, 0.025f, 0.20f);
            break;

        case SoundEffect::TilePlace:
        case SoundEffect::TilePlaceA:
            // Bright, crystalline droplet / high marimba (Blue / Dot)
            trigger_voice(660.0f, -100.0f, 0.25f, 0.09f, 0.35f);
            break;

        case SoundEffect::TilePlaceB:
            // Warm, grounded wooden knock / solid block tap (Red / Wall)
            trigger_voice(360.0f, -200.0f, 0.25f, 0.08f, 0.15f);
            break;

        case SoundEffect::TileClear:
            // Airy descending pop / whoosh
            trigger_voice(480.0f, -1200.0f, 0.18f, 0.06f, 0.0f);
            break;

        case SoundEffect::ActionBlocked:
            // Soft low muted double-knock (attempting to edit fixed clue)
            trigger_voice(160.0f, -400.0f, 0.22f, 0.045f, 0.10f);
            break;

        case SoundEffect::HintRequested:
            // Magical 3-note ascending harp sparkle
            trigger_voice(1046.50f, 80.0f, 0.18f, 0.32f, 0.20f); // C6
            trigger_voice(1318.51f, 80.0f, 0.20f, 0.38f, 0.25f); // E6
            trigger_voice(1567.98f, 80.0f, 0.22f, 0.44f, 0.30f); // G6
            break;

        case SoundEffect::NumberSatisfied:
            // Pure crystalline bell chime (Vision clue completed)
            trigger_voice(1174.66f, 0.0f, 0.22f, 0.28f, 0.40f); // D6
            break;

        case SoundEffect::PuzzleSolved:
            // Grand triumphant major chord fanfare
            trigger_voice(523.25f, 0.0f, 0.24f, 0.70f, 0.15f);  // C5
            trigger_voice(659.25f, 0.0f, 0.24f, 0.75f, 0.18f);  // E5
            trigger_voice(783.99f, 0.0f, 0.25f, 0.80f, 0.20f);  // G5
            trigger_voice(1046.50f, 0.0f, 0.27f, 0.90f, 0.25f); // C6
            trigger_voice(1318.51f, 0.0f, 0.22f, 0.95f, 0.30f); // E6
            break;

        case SoundEffect::MenuSelect:
            // Sharp, pleasant confirmation chime
            trigger_voice(880.0f, 0.0f, 0.22f, 0.12f, 0.25f); // A5
            break;

        case SoundEffect::MenuBack:
            // Soft muted cancellation tone
            trigger_voice(293.66f, -150.0f, 0.20f, 0.09f, 0.10f); // D4
            break;

        case SoundEffect::VolumeToggle:
            // Crisp toggle blip
            trigger_voice(740.0f, 200.0f, 0.18f, 0.05f, 0.15f);
            break;
    }
    unlock();
}

void AudioManager::sdl_audio_callback(void* userdata, uint8_t* stream, int len) {
    auto* self = static_cast<AudioManager*>(userdata);
    self->audio_callback(stream, len);
}

void AudioManager::audio_callback(uint8_t* stream, int len) {
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int num_frames = len / sizeof(int16_t) / 2; // Stereo samples

    // 1. Read MP3 audio frames if music enabled
    drmp3_uint64 frames_read = 0;
    if (music_enabled_ && mp3_loaded_) {
        frames_read = drmp3_read_pcm_frames_s16(&mp3_decoder_, num_frames, buffer);
        if (frames_read < static_cast<drmp3_uint64>(num_frames)) {
            // Loop track seamlessly from start
            drmp3_seek_to_pcm_frame(&mp3_decoder_, 0);
            drmp3_uint64 more = drmp3_read_pcm_frames_s16(&mp3_decoder_, num_frames - frames_read, buffer + frames_read * 2);
            frames_read += more;
        }
    }

    // Silence any remaining unread buffer
    if (frames_read < static_cast<drmp3_uint64>(num_frames)) {
        memset(buffer + frames_read * 2, 0, (num_frames - frames_read) * 2 * sizeof(int16_t));
    }

    // 2. Synthesize & mix active procedural SFX voices on top
    float dt = 1.0f / static_cast<float>(sample_rate_);
    for (int i = 0; i < num_frames; ++i) {
        float sfx_mix = 0.0f;
        for (auto& v : sfx_voices_) {
            if (!v.active) continue;

            v.amplitude -= dt / v.decay;
            if (v.amplitude <= 0.001f) {
                v.amplitude = 0.0f;
                v.active = false;
                continue;
            }

            v.frequency += v.freq_slide * dt;
            if (v.frequency < 20.0f) v.frequency = 20.0f;

            float s1 = std::sin(v.phase);
            float s2 = std::sin(v.phase * 2.0f);
            float sample = (s1 + v.harmonic2 * s2) * v.amplitude;

            v.phase += 2.0f * 3.14159265f * v.frequency * dt;
            if (v.phase >= 2.0f * 3.14159265f) v.phase -= 2.0f * 3.14159265f;

            sfx_mix += sample;
        }

        if (sfx_mix != 0.0f) {
            int32_t sfx_sample = static_cast<int32_t>(sfx_mix * 22000.0f);
            int32_t l = buffer[i * 2 + 0] + sfx_sample;
            int32_t r = buffer[i * 2 + 1] + sfx_sample;
            buffer[i * 2 + 0] = static_cast<int16_t>(std::clamp<int32_t>(l, -32768, 32767));
            buffer[i * 2 + 1] = static_cast<int16_t>(std::clamp<int32_t>(r, -32768, 32767));
        }
    }
}

} // namespace ohpsp::engine
