# NESPRESSO NES Emulator

**Brewing Nostalgia One Frame at a Time!*tm**

![NESPRESSO](assets/nespresso_logo.png)

```
  _______  _____  _____  _____  _____  _     _  _______
 |__   __||  _  |/  ___||  _  |/  __ \| \   / |/__   __/
    | |   | | | |\ `--. | | | || /  \/|  \_/  |   | |
    | |   | | | | `--. \| | | || |    |       |   | |
    | |   | |_| |/\__/ /\ \_/ /| \__/\| |     |   | |
    |_|   \___/ \____/  \___/  \____/\|_|     |   | |_

        ___    ___    ___    ___
       /  /   /  /   /  /   /  /
      /  /   /  /   /  /   /  /__
     /  /   /  /   /  /   /_____/
    /__/   /__/   /__/   __/ /_/
```

---

## Overview

NESPRESSO is a high-performance, cycle-accurate Nintendo Entertainment System (NES) emulator. Built with pure C/C++ for maximum portability and minimal dependencies, it brings classic 8-bit gaming to modern platforms with the precision of a finely crafted espresso.

- **Bold**: Accurate emulation of 6502 CPU, PPU, and APU
- **Fast**: Cycle-accurate core with modern optimizations
- **Pure Nostalgia**: Authentic还原 of the NES experience

---

## Features

- **Full 6502 CPU Emulation**: All 151 official opcycles with cycle timing
- **Accurate PPU**: 256x240 output with proper sprite rendering, scrolling, and mirroring
- **Complete APU**: All audio channels - 2 Pulse, Triangle, Noise, and DMC at 44.1kHz
- **Mapper Support**: Mapper 0/1/2/3/4/7 (covers ~90% of NES library)
- **Save States**: Load and save your progress anytime
- **SRAM Support**: Battery backup saves for compatible games
- **Cross-Platform**: Windows, Linux, and macOS support

---

## System Requirements

- **CPU**: x86-64 or ARM64 processor (2020+ recommended)
- **RAM**: 512 MB minimum
- **OS**:
  - Windows 10/11
  - Linux (Ubuntu 20.04+, Arch, etc.)
  - macOS 11+

---

## Quick Start (Windows)

### ⚡ **Recommended: MinGW/MSYS2 (Easy Build)**

1. **Install MSYS2** from https://www.msys2.org/

2. **Open "MSYS2 MinGW 64-bit" terminal** and run:
   ```bash
   pacman -Syu                    # Update (may need to restart)
   pacman -S mingw-w64-x86_64-gcc
   pacman -S mingw-w64-x86_64-SDL2
   pacman -S mingw-w64-x86_64-make
   ```

3. **Navigate to project and build:**
   ```bash
   cd /c/path/to/nespresso
   make
   ```

4. **Run:**
   ```bash
   ./NESPRESSO.exe path/to/game.nes
   ```

*For detailed MinGW guide: See [MINGW_BUILD.md](MINGW_BUILD.md)*

---

### Alternative: Visual Studio

1. Install Visual Studio 2019 or newer
2. Open `NESPRESSO.sln`
3. F5 to build and run

*For detailed VS guide: See [WINDOWS_BUILD.md](WINDOWS_BUILD.md)*

---

### Using CMake (Any Compiler)

```cmd
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc
cmake --build . --config Release
NESPRESSO.exe ../roms/yourgame.nes
```

### Usage

```cmd
# Load a ROM
NESPRESSO.exe path/to/game.nes

# Controls:
# Arrow Keys  - D-Pad (Up/Down/Left/Right)
# Z           - A Button
# X           - B Button
# Enter       - Start
# Tab         - Select

# Save State: F5
# Load State: F9
# Reset:      F1
# Exit:       ESC
```

---

## Quick Start (Linux/macOS)

### Prerequisites

**Ubuntu/Debian:**
```bash
sudo apt install build-essential cmake libsdl2-dev
```

**Arch Linux:**
```bash
sudo pacman -S base-devel cmake sdl2
```

**macOS (via Homebrew):**
```bash
brew install cmake sdl2
```

### Building & Running

```bash
# Clone and build
git clone https://github.com/yourusername/nespresso-nes-emulator.git
cd nespresso-nes-emulator
mkdir build && cd build
cmake ..
make -j$(nproc)

# Run
./NESPRESSO ../roms/your_game.nes
```

---

## Project Structure

```
nespresso/
├── src/
│   ├── cpu/          # 6502 CPU emulation
│   ├── ppu/          # Picture Processing Unit
│   ├── apu/          # Audio Processing Unit
│   ├── cartridge/    # ROM loading and parsing
│   ├── mapper/       # Mapper implementations
│   ├── input/        # Controller handling
│   ├── memory/       # Memory mapping and bus
│   └── platform/     # OS-specific code
├── roms/             # Place your .nes ROMs here
├── build/            # Build output
└── docs/             # Documentation
```

---

## Supported Mappers

| Mapper | Name                     | Example Games                               |
|--------|--------------------------|---------------------------------------------|
| 0      | NROM                     | Donkey Kong, Ice Climber                    |
| 1      | MMC1                     | Zelda, Metroid, Final Fantasy               |
| 2      | UxROM                    | Mega Man, Castlevania, Contra               |
| 3      | CNROM                    | Kid Icarus, Arkanoid                        |
| 4      | MMC3                     | Super Mario Bros 3, Kirby's Adventure       |
| 7      | AxROM                    | Battletoads, Marble Madness                  |

---

## Performance

| Platform | Avg FPS | CPU Usage |
|----------|---------|-----------|
| Windows 10 (i7-10700) | 60 | ~2% |
| Ubuntu 22.04 (Ryzen 5600) | 60 | ~2.5% |
| macOS 13 (M1) | 60 | ~3% |

---

## Development

### Building in Debug Mode

```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
make -j$(nproc)
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

---

## Contributing

Pull requests are welcome! Please see [CONTRIBUTING.md](docs/CONTRIBUTING.md) for guidelines.

## License

This project is licensed under the MIT License - see [LICENSE](LICENSE) for details.

## Credits

- Inspired by classic NES emulator design patterns
- NES Dev wiki for comprehensive documentation
- The amazing NES homebrew community

---

**Brewed with `love` and `caffeine` by the NESPRESSO Team**

*"Wake up and smell the pixels!"*
