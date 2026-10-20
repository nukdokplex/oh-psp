# AGENTS.md — Oh PSP

Instructions and reference for AI coding agents working on **Oh PSP**.

---

## 1. Project & Copyright Constraints (CRITICAL)

- **Identity**: `Oh PSP` is a modular logic puzzle collection for the Sony PlayStation Portable (PSP) written in C++20 with SDL2, licensed under MIT.
- **Copyright & Naming Policy**:
  - **NEVER** mention or reference the original proprietary mobile game names or trademarks in code, comments, commit messages, UI strings, or documentation.
  - Always use neutral/generic mathematical puzzle names:
    - **Takuzu** — the binary grid puzzle (two colors/states, balance, no trios, unique rows/cols).
    - **Vision** — the line-of-sight puzzle (numbered blue dots counting visible dots along rows/columns before hitting walls).
  - Main menu provides game selection between **Takuzu**, **Vision**, and future modular games.

---

## 2. Toolchain & Nix Environment

The project uses Nix flakes with `flake-parts` and `pspdev-nix`.

- **Cachix Binary Cache** (prevents building toolchain from source):
  - Cache: `https://pspdev.cachix.org`
  - Public key: `pspdev.cachix.org-1:lFw1M0EYJeN3Y2xHR7spiuPmThrNDXo8Z9I0Jgzig/0=`
- **Flake Input**: `pspdev.url = "github:pspdev/pspdev-nix"`

### Developer Commands

```sh
# 1. Build & run native desktop version (instant UI debugging & testing):
nix run .#desktop
# Or manually with CMake:
cmake -B build-host -DPLATFORM_PSP=OFF -DBUILD_TESTS=ON
cmake --build build-host
./build-host/oh_psp

# 2. Run unit tests on host:
ctest --test-dir build-host --output-on-failure

# 3. Cross-compile for PSP (generates result/EBOOT.PBP):
nix build .#default --accept-flake-config
# Or inside devshell:
nix develop --accept-flake-config --command sh -c "psp-cmake -B build-psp && cmake --build build-psp"

# 4. Run PSP build in PPSSPP emulator:
nix develop --command ppsspp result/EBOOT.PBP
```

---

## 3. Architecture & Modular Game Framework

The repository is organized to allow adding new puzzle games without altering existing game code or core engine infrastructure.

### Directory Boundaries

```
├── flake.nix              # Flake-parts based Nix flake with PSP & host toolchains
├── CMakeLists.txt         # Root build configuration (detects PSP vs Host)
├── src/
│   ├── engine/            # Shared platform layer (SDL2, input, audio, save data)
│   │   ├── display.hpp    # Fixed 480x272 virtual / physical viewport
│   │   ├── input.hpp      # Unified input (PSP buttons mapped to host keys)
│   │   └── registry.hpp   # IGameModule interface & GameRegistry
│   ├── menu/              # Main menu: hub for selecting games & grid sizes
│   └── games/             # Self-contained game modules
│       ├── takuzu/
│       │   ├── core/      # Pure math solver/generator (ZERO SDL/PSP dependencies)
│       │   └── ui/        # TakuzuModule (implements IGameModule)
│       └── vision/
│           ├── core/      # Pure math solver/generator (ZERO SDL/PSP dependencies)
│           └── ui/        # VisionModule (implements IGameModule)
└── tests/                 # Host unit tests for core math & algorithms
    ├── test_takuzu.cpp
    └── test_vision.cpp
```

### Modular Game Contract (`IGameModule`)

Each game in `src/games/<name>/` implements `IGameModule`:
```cpp
class IGameModule {
public:
    virtual ~IGameModule() = default;
    virtual const char* get_id() const = 0;       // e.g. "takuzu"
    virtual const char* get_title() const = 0;    // e.g. "Takuzu"
    virtual std::vector<GamePreset> get_presets() const = 0; // Available grid sizes
    virtual void start_game(const GamePreset& preset) = 0;
    virtual void handle_input(const InputState& input) = 0;
    virtual void update(float dt) = 0;
    virtual void render(SDL_Renderer* renderer) = 0;
    virtual bool is_completed() const = 0;
};
```

### How to Add a New Game

1. Create `src/games/<new_game>/core/` for the puzzle rules, solver, and generator.
2. Add comprehensive unit tests in `tests/test_<new_game>.cpp` (must pass on host).
3. Create `src/games/<new_game>/ui/` implementing `IGameModule`.
4. Register the module in `GameRegistry` inside `src/menu/` or game bootstrap.

---

## 4. Puzzle Math & Rule Specifications

### Takuzu
- **Grid**: Even sizes ($N \times N$, typical: 4, 6, 8, 10, 12).
- **Cells**: Empty (0), Blue/Color A (1), Red/Color B (2).
- **Rules**:
  1. No three adjacent cells in any row or column may have the same color.
  2. Each row and column must contain an equal number of each color ($N / 2$).
  3. Every completed row must be unique; every completed column must be unique.
- **Generation**: Generate a full valid board, then systematically remove numbers while ensuring a unique solution via solver backtracking.

### Vision
- **Grid**: $N \times N$ (typical: 4, 5, 6, 7, 8, 9).
- **Cells**: Empty, Wall (Red), Dot (Blue), Numbered Dot (Blue with count $K$).
- **Rules**:
  1. Numbered blue dots indicate exactly how many *other* blue dots are visible horizontally and vertically.
  2. Visibility rays extend up, down, left, right from the tile and are blocked by walls (red dots) or grid edges.
  3. A dot cannot see beyond a wall.
  4. All dots must have visibility consistent with their visible ray counts upon puzzle completion.
- **Generation**: Place walls and dots, compute sight numbers, test for unique solvability using deduction rules before backtracking.

---

## 5. PSP Hardware Quirks & Agent Conventions

- **Screen**: Strictly **480 × 272** pixels, 16:9 aspect ratio.
  - On host builds, scale up cleanly by integer multipliers (e.g. 2x = 960x544) to keep pixel aspect correct.
- **Memory Footprint**: PSP-1000 has only **32 MB** of total RAM (approx 20-24 MB accessible to user homebrew).
  - Avoid large heap allocations during gameplay.
  - Pre-allocate puzzle scratch buffers and solver states; do not perform deep unchecked recursion in board generators.
- **MIPS Allegrex Architecture Quirks**:
  - **Compiler Flag `-G0`**: Always compile with `-G0` under PSP (`add_compile_options(-G0)`). Disables small-data optimization to prevent global pointer table overflow (`R_MIPS_GPREL16 relocation truncated`).
  - **Type Widths**: On MIPS `psp-gcc`, `int32_t` is defined as `long int` (not `int`). Template deduction in functions like `std::clamp` will fail with mixed types; always specify types explicitly: `std::clamp<int32_t>(val, -32768, 32767)`.
  - **Working Directory (`chdir`)**: PSP OS launches EBOOTs without setting CWD to the game folder. On startup, `main.cpp` must extract the directory from `argv[0]` and call `chdir(dir)`.
- **Nix Sandbox Rule**: Nix flakes only copy files tracked by Git (`src = ./.`). Always register new files via `git add -N` before running `nix build`, otherwise the compiler inside the sandbox will fail to find them.
- **Zero-Dependency Cores**: Keep `src/games/*/core` 100% free of OS/graphics headers (`<SDL.h>`, `<psp*.h>`). This allows instantaneous unit testing via `ctest` on host.

---

## 6. Audio Architecture & Sound Design

- **Self-Contained EBOOT (Embedded Assets)**:
  - All background music tracks are embedded directly into the `.rodata` section via GNU assembler `.incbin` (`src/engine/embedded_music.S`).
  - Single-file `EBOOT.PBP` distribution avoids Memory Stick read latency, saves battery, and eliminates PSP XMB "Corrupted Data" errors caused by extra subfolders.
- **MP3 Decoding**:
  - Implemented using single-header `dr_mp3.h` with `drmp3_init_memory`.
  - Streamed in chunks inside the SDL audio callback; loops seamlessly.
- **Procedural Real-Time SFX Synthesizer**:
  - All gameplay sound effects are 100% mathematically synthesized in `audio_callback` (harmonic overtones, frequency slides, exponential decay envelopes).
  - Fully legal, royalty-free, zero disk I/O, zero external audio asset dependencies.
- **Hardware Audio Parameters**:
  - Frequency: strictly **44100 Hz** (matches PSP hardware `sceAudioChReserve`).
  - Format: `AUDIO_S16SYS` (16-bit signed stereo).
  - Buffer: **1024** samples (must be a multiple of 64 bytes for PSP hardware alignment).

---

## 7. Dual-Cursor UX Model (Focus vs. Pointer)

The game provides two complementary navigation systems that coexist seamlessly:

1. **Селектор (`Focus`)**:
   - Discrete cell/item highlight frame controlled by the **D-Pad** (or Arrow keys).
   - Jumps between grid tiles and menu cards one step at a time.
2. **Указатель (`Pointer`)**:
   - Continuous desktop-style arrow cursor controlled by the **Analog Stick** (or PC Mouse).
   - Floats above all UI layers with a subtle drop shadow (`RenderUtils::draw_pointer`).
   - Filtered with a deadzone of `0.18` to neutralize physical stick drift on older PSPs.
3. **Hybrid Focus Priority Rule (CRITICAL)**:
   - When hovering over an interactive cell/card, the Pointer pulls the Focus to that element **ONLY if the pointer is actively moving** (`input.pointer_moved == true`).
   - When the Pointer is stationary, it **never** steals or locks the Focus; D-Pad navigation maintains complete, unhindered priority.
4. **Button Prompt Convention**:
   - Always display the Triangle prompt as **`[Tri]`** (never `[/^]` or `[/\]`) for readability on the low-resolution 480×272 screen.

