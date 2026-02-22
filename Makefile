# NESPRESSO NES Emulator - Makefile
# Brewing Nostalgia One Frame at a Time!

# Detect OS
UNAME_S := $(shell uname -s)

# Compiler settings
CC = gcc
CFLAGS = -O3 -Wall -Wextra -Isrc -I. -Iinclude -march=native

# SDL2 configuration
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null || pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null || pkg-config --libs sdl2 2>/dev/null)

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    # Windows (Make for Windows)
    WINDRES = windres
    LDFLAGS = -mwindows
    TARGET = NESPRESSO.exe
    ifeq ($(SDL_CFLAGS),)
	SDL_CFLAGS = -IC:/msys64/mingw64/include/SDL2 -IC:/SDL2/include -Dmain=SDL_main
    endif
    ifeq ($(SDL_LIBS),)
	SDL_LIBS = -LC:/msys64/mingw64/lib -LC:/SDL2/lib/x64 -lmingw32 -lSDL2main -lSDL2 -mwindows
    endif
    LDFLAGS += -mconsole $(SDL_LIBS)
else ifeq ($(UNAME_S),Linux)
    # Linux
    TARGET = NESPRESSO
    LDFLAGS = $(SDL_LIBS) -lm -lpthread
else ifeq ($(UNAME_S),Darwin)
    # macOS
    TARGET = NESPRESSO
    LDFLAGS = $(SDL_LIBS) -framework Cocoa -lm
else
    # Assume Linux/Unix as default
    TARGET = NESPRESSO
    LDFLAGS = $(SDL_LIBS) -lm -lpthread
endif

# Combine flags
CFLAGS += $(SDL_CFLAGS)

# Source files
SRCS = src/main.c \
       src/cpu/cpu.c \
       src/ppu/ppu.c \
       src/apu/apu.c \
       src/cartridge/rom.c \
       src/mapper/mapper.c \
       src/input/input.c \
       src/memory/bus.c \
       src/platform/platform.c

# Object files
OBJS = $(SRCS:.c=.o)

# Executable name
TARGET = NESPRESSO.exe

# Icon resource (optional)
ICON_RES = icon.res

.PHONY: all clean release debug run help install uninstall

# Default target
all: $(TARGET)

# Icons (if you have one)
icon.res: icon.ico
	$(WINDRES) -i icon.rc -o icon.res -O coff

$(TARGET): $(OBJS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJS) $(LDFLAGS) -o $(TARGET)
	@echo "Build complete! Run with: $(TARGET) game.nes"

%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Release build with maximum optimization
release: CFLAGS += -O3 -flto -DNDEBUG
release: clean $(TARGET)

# Debug build
debug: CFLAGS += -g -O0 -DDEBUG
debug: clean $(TARGET)

# Run with a test ROM (create dummy)
run: $(TARGET)
	@echo "Please provide a ROM file to run"
	@echo "Usage: make run ROM=yourgame.nes"
ifdef ROM
	./$(TARGET) $(ROM)
endif

# Clean build artifacts
clean:
	@echo "Cleaning..."
	@rm -f $(OBJS) $(TARGET)
	@echo "Clean complete"

# Help
help:
	@echo "NESPRESSO NES Emulator - Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the emulator (default)"
	@echo "  release   - Build with optimizations"
	@echo "  debug     - Build with debug symbols"
	@echo "  clean     - Remove build artifacts"
	@echo "  run       - Run emulator (specify ROM=game.nes)"
	@echo ""
	@echo "Prerequisites:"
	@echo "  - gcc"
	@echo "  - SDL2 development libraries (libsdl2-dev on Debian/Ubuntu, sdl2 on Arch)"
