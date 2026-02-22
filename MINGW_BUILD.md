# Building NESPRESSO with MinGW on Windows

**Brewing Nostalgia One Frame at a Time!**

## Quick Start (MSYS2 MinGW 64-bit)

### 1. Install MSYS2

1. Download MSYS2 from https://www.msys2.org/
2. Run the installer and follow the prompts
3. Launch "MSYS2 MinGW 64-bit" terminal (not MSYS2 MSYS)

### 2. Install Required Packages

```bash
pacman -Syu           # Update packages (may need to restart terminal)
pacman -S mingw-w64-x86_64-gcc
pacman -S mingw-w64-x86_64-SDL2
pacman -S mingw-w64-x86_64-make
pacman -S mingw-w64-x86_64-diffutils
```

### 3. Build

```bash
cd /c/path/to/nespresso
make
```

### 4. Run

```bash
./NESPRESSO.exe path/to/game.nes
```

## Alternative: Using Make Directly

If you have MinGW installed elsewhere:

### Prerequisites

- MinGW-w64 (`gcc`, `make`)
- SDL2 libraries

### Finding SDL2

The Makefile looks for SDL2 in these locations (in order):
1. Via `sdl2-config` or `pkg-config`
2. `C:\msys64\mingw64\include\SDL2`
3. `C:\SDL2\include`

To install SDL2 manually:

1. Download SDL2 development libraries:
   https://github.com/libsdl-org/SDL/releases
   Download: SDL2-devel-mingw.zip

2. Extract to `C:\SDL2\`

3. Ensure libraries are in `C:\SDL2\lib\x64\`

### Build Commands

```cmd
REM In Windows Command Prompt or PowerShell
cd path\to\nespresso

REM Build
mingw32-make

REM Release build
mingw32-make release

REM Debug build
mingw32-make debug

REM Clean
mingw32-make clean

REM Run
NESPRESSO.exe game.nes
```

### Using MSYS2 MinGW Make

If using MSYS2, the `make` command should work directly:

```bash
make

# Or explicitly use mingw32-make
mingw32-make

# Run
./NESPRESSO.exe game.nes
```

## Building with CMake (MinGW)

```bash
# In MSYS2 MinGW 64-bit terminal
cd /c/path/to/nespresso

# Configure for MinGW
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++

# Build
cmake --build build --config Release

# Run
build/NESPRESSO.exe path/to/game.nes
```

## Using Visual Studio Code

1. Install VS Code from https://code.visualstudio.com/

2. Install extensions:
   - C/C++
   - CMake Tools

3. Open the nespresso folder

4. Select "Yes" when prompted to configure CMake

5. Click the status bar and select "MinGW GCC" as the kit

6. Press F7 to build

7. Set launch configuration:

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Debug",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/NESPRESSO.exe",
            "args": ["roms/yourgame.nes"],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "gdb.exe"
        }
    ]
}
```

## Troubleshooting

### SDL2 Not Found

**Error:** `fatal error: SDL2/SDL.h: No such file or directory`

**Solutions:**

1. Install SDL2 via MSYS2:
   ```bash
   pacman -S mingw-w64-x86_64-SDL2
   ```

2. Or set SDL2 path in Makefile:
   ```makefile
   SDL_CFLAGS = -IC:/Your/SDL2/Path/include
   SDL_LIBS = -LC:/Your/SDL2/Path/lib/x64 -lSDL2main -lSDL2 -mwindows
   ```

3. Or set environment variable:
   ```cmd
   set SDL2_DIR=C:\SDL2
   ```

### GCC Not Found

**Error:** `gcc: command not found`

**Solutions:**

1. Make sure you're using the "MSYS2 MinGW 64-bit" terminal, not MSYS2 MSYS

2. Add MinGW to PATH:
   ```cmd
   set PATH=C:\msys64\mingw64\bin;%PATH%
   ```

3. Verify installation:
   ```cmd
   gcc --version
   make --version
   ```

### Missing DLLs

**Error:** `libgcc_s_seh-1.dll was not found` or `libwinpthread-1.dll was not found`

**Solutions:**

1. Copy required MinGW DLLs to your build directory:
   ```cmd
   copy C:\msys64\mingw64\bin\*.dll .
   ```

2. Or add MinGW bin to your system PATH

### Make: 'mingw32-make' Not Found

**Solutions:**

1. Use `make` instead (in MSYS2 terminal)

2. Or install MinGW Make:
   ```bash
   pacman -S mingw-w64-x86_64-make
   ```

3. Or create a symlink:
   ```cmd
   mklink C:\msys64\mingw64\bin\make.exe C:\msys64\mingw64\bin\mingw32-make.exe
   ```

### Unicode Path Issues

**Error:** Build fails with Japanese/Chinese paths

**Solution:**
```cmd
REM Set code page to UTF-8
chcp 65001
mingw32-make
```

## Package Structure

```
NESPRESSO/
├── NESPRESSO.exe        # Main executable
├── *.dll                # Required DLLs (copy from MinGW bin if needed)
├── roms/                # Put your .nes files here
└── save/                # Save states appear here
```

## Command Line Options

```
NESPRESSO <rom_file> [options]

Options:
  -1, -2, -3, -4      Display scale factor (default: 3)
  -f, --fullscreen    Start in fullscreen mode
  --no-audio          Disable audio output
  -h, --help          Show help message

Controls:
  Arrow Keys    D-Pad
  Z             A Button
  X             B Button
  Enter         Start
  Tab           Select

Hotkeys:
  F1    Reset
  F5    Save State
  F7    Change Save Slot (0-9)
  F9    Load State
  F11   Toggle Fullscreen
  ESC   Exit
```

## Performance Tips

1. Use `make release` for optimized builds
2. Ensure you're building for the correct architecture (x64)
3. Disable VSYNC in SDL if tearing occurs:
   ```c
   SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
   ```

## Testing

Run the official NES test ROMs:

```bash
./NESPRESSO.exe roms/cpu.nes
./NESPRESSO.exe roms/ppu.nes
```

Download test ROMs:
- https://www.nesdev.org/wiki/Emulator_tests
- https://github.com/christopherpow/nes-test-roms

## Tips for Distribution

To create a standalone package:

```cmd
mkdir dist
copy NESPRESSO.exe dist\
copy C:\msys64\mingw64\bin\*.dll dist\
mkdir dist\roms
mkdir dist\save
copy README.txt dist\
```

Then ZIP the `dist` folder.

Happy brewing!
