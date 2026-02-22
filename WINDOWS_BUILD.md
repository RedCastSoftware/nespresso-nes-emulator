# Building NESPRESSO on Windows

**Brewing Nostalgia One Frame at a Time!**

## Prerequisites

### Method 1: Using Visual Studio (Recommended)

1. **Install Visual Studio** (2019 or newer)
   - Download from: https://visualstudio.microsoft.com/downloads/
   - During installation, select "Desktop development with C++"

2. **Install vcpkg** (C++ package manager)
   ```cmd
   git clone https://github.com/Microsoft/vcpkg.git C:\vcpkg
   cd C:\vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

3. **Install SDL2 via vcpkg**
   ```cmd
   .\vcpkg install sdl2:x64-windows
   ```

### Method 2: Using MinGW-w64

1. **Install MSYS2**
   - Download from: https://www.msys2.org/
   - Run the installer and follow the prompts

2. **Install MinGW packages**
   ```bash
   # Open MSYS2 MinGW 64-bit terminal
   pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-SDL2
   ```

3. **Add to PATH** (optional but recommended)
   - Add `C:\msys64\mingw64\bin` to your system PATH

### Method 3: Using Precompiled SDL2

1. **Download SDL2 development libraries**
   - Go to: https://github.com/libsdl-org/SDL/releases
   - Download the latest SDL2-devel-VC.zip (for Visual Studio)
   - OR SDL2-devel-mingw.zip (for MinGW)

2. **Extract to a convenient location**
   - For example: `C:\SDL2`

## Building with CMake

### Visual Studio + CMake

```cmd
# Open Command Prompt (Developer Command Prompt for VS)
cd path\to\nespresso

# Configure
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake -A x64

# Build
cmake --build build --config Release

# Run
build\Release\NESPRESSO.exe path\to\game.nes
```

### Visual Studio IDE (with integrated CMake support)

1. Open Visual Studio
2. Select "Open a local folder"
3. Navigate to the `nespresso` directory and open it
4. The CMakeLists.txt will be automatically detected
5. Click on the "Select Startup Item" dropdown and select `NESPRESSO.exe`
6. Press F5 to build and run

### Using CMake GUI

1. Install CMake from https://cmake.org/download/
2. Open CMake (cmake-gui)
3. Set "Where is the source code" to the nespresso directory
4. Set "Where to build binaries" to a build directory
5. Click "Configure"
6. Choose your generator (e.g., "Visual Studio 17 2022")
7. Click "Generate"
8. Open the generated project file (`build/NESPRESSO.sln`)

### MinGW-w64 + CMake

```cmd
# Open MSYS2 MinGW 64-bit terminal
cd /c/path/to/nespresso

# Configure
cmake -B build -S . -G "MinGW Makefiles" -DCMAKE_C_COMPILER=gcc -DCMAKE_CXX_COMPILER=g++

# Build
cmake --build build --config Release

# Run
build/NESPRESSO.exe path/to/game.nes
```

## Building with Visual Studio Directly (Alternative)

If you prefer to use Visual Studio project files directly:

### Creating Project Files

1. Open Visual Studio
2. Create a new "Empty Project"
3. Project name: `NESPRESSO`
4. Location: Set to the nespresso directory (or create a `build` subdirectory)

### Adding Source Files

1. Right-click on the project in Solution Explorer
2. Add → Existing Item
3. Select all `.c` files from `src/` directory
4. Organize into folders (CPU, PPU, APU, etc.) by adding Filter folders

### Configuring Project Properties

1. Right-click project → Properties

2. **C/C++ → General → Additional Include Directories**
   - Add: `.;src;C:\vcpkg\installed\x64-windows\include` (or your SDL2 path)

3. **C/C++ → Preprocessor → Preprocessor Definitions**
   - Add: `_CRT_SECURE_NO_WARNINGS`

4. **Linker → General → Additional Library Directories**
   - Add: `C:\vcpkg\installed\x64-windows\lib` (or your SDL2 lib path)

5. **Linker → Input → Additional Dependencies**
   - Add: `SDL2.lib;SDL2main.lib`

6. **All Configurations → Character Set**
   - Set to: "Not Set" or "Use Multi-Byte Character Set"

### Building

1. Right-click project → Build
2. Or press Ctrl+Shift+B

### Running

1. Set Command Arguments: Right-click project → Properties → Debugging → Command Arguments
   - Add your ROM filename: `path\to\game.nes`

2. Press F5 to run

or run from command line:
```cmd
x64\Release\NESPRESSO.exe path\to\game.nes
```

## Copying SDL2 DLLs

When building with Visual Studio, you'll need to copy `SDL2.dll` to your executable directory:

```cmd
copy C:\vcpkg\installed\x64-windows\bin\SDL2.dll build\Release\
```

Or add a post-build event in Visual Studio:
```
copy /Y "$(VCPKG_ROOT)\installed\x64-windows\bin\SDL2.dll" "$(OutputPath)"
```

## Troubleshooting

### SDL2 Not Found

**Error:** `Could not find SDL2`

**Solution:**
- Make sure vcpkg is properly integrated
- Check SDL2 installation path
- Try manually specifying SDL2 path:
  ```cmd
  cmake -DS DL2_DIR=C:/SDL2 ..
  ```

### Missing SDL2.dll

**Error:** `SDL2.dll was not found`

**Solution:**
- Copy SDL2.dll to the same directory as your executable
- Or add SDL2's bin directory to your PATH

### Compiler Errors

**Error:** Various compilation errors

**Solution:**
- Make sure you have C11 support (VS2019+)
- Set C language standard in CMake: `set(CMAKE_C_STANDARD 11)`
- Check that all source files are being compiled

### Linker Errors

**Error:** `unresolved external symbol SDL_*`

**Solution:**
- Verify SDL2 library directory is correct
- Check that you're linking `SDL2main.lib` and `SDL2.lib`
- For x64 build, use x64 SDL2 libraries

### Performance Issues

**Issue:** Emulation runs too fast/slow

**Solution:**
- Release builds are faster: Use `cmake --build build --config Release`
- Check that frame timing is working (VSYNC should limit FPS to 60)

## Testing

After building successfully, test with a simple ROM:

```cmd
NESPRESSO.exe nes\cpu.nes
```

You can find test ROMs at:
- https://www.nesdev.org/wiki/Emulator_tests
- https://github.com/christopherpow/nes-test-roms

## Package Structure for Distribution

```
NESPRESSO/
├── NESPRESSO.exe
├── SDL2.dll
├── roms/               # Put your .nes files here
├── save/               # Save states and SRAM appear here
└── README.txt
```

## Command Line Options

```
NESPRESSO <rom_file> [options]

Options:
  -1, -2, -3, -4      Display scale (default: 3)
  -f, --fullscreen    Start in fullscreen
  --no-audio          Disable audio
  -h, --help          Show help
```

## Further Development

- To enable debug symbols: `cmake -DCMAKE_BUILD_TYPE=Debug ..`
- To get verbose build output: `cmake --build . --verbose`
- Visual Studio Code: Install "CMake Tools" extension and use the built-in CMake support

Happy brewing!
