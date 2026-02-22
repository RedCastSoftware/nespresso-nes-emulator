# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

```markdown
# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this codebase.
```

## Repository Overview

**NESPRESSO** is a cycle-accurate Nintendo Entertainment System (NES) emulator written in pure C with SDL2 rendering/audio. The project is structured around the component-oriented NES hardware: CPU, PPU, APU, Cartridge loader, Mapper system, Input handling, Memory bus, and Platform abstraction layer.

### Project Characteristics

- **Language**: C99 only (no C++)
- **Dependencies**: SDL2 for graphics/audio
- **Platform**: Windows (MinGW/MSVC), Linux, macOS
- **Architecture**: Component-based NES hardware emulation
- **Size**: ~6,000 lines of code across 10 modules

---

## Commands

### Building the Project

```bash
# MinGW (Windows)
cd /c/path/to/nespresso-nes-emulator
make

# CMake (cross-platform)
mkdir build && cd build
cmake .. && make -j$(nproc)

# Visual Studio
# Open NESPRESSO.sln and build in Visual Studio (F5)
```

### Running Tests

```bash
# Currently no automated tests. To create tests:
# - Use Google Test or CTest framework
# - Create test roms in tests/ directory
# - Write unit tests for each module
```

### Common Development Commands

```bash
# Rebuild after changes
make

# Clean build artifacts
make clean

# Run with current code immediately
make && ./NESPRESSO.exe yourgame.nes

# Check for memory leaks with Valgrind (Linux)
valgrind --leak-check=full ./NESPRESSO.exe

# Generate assembly code (check code quality)
objdump -d NESPRESSO.exe > dump.txt
less dump.txt

# Strip debug symbols for release build
strip NESPRESSO.exe
```

### Linting/Static Analysis

```bash
# clang-analyze src/**/*.c 2>&1 | head -50
cppcheck src/{cpu,ppu,apu,mapper}/*.c 2>&1 | head -30

# Check for common issues
grep -n -r "TODO\|FIXME\|HACK\|XXX" src/ -i

# Check for memory leaks from static allocations
grep -n "static.*\[\[0-9\]*" src/ -i | wc -l
```

---

## Code Architecture

### Module Dependencies

```
┌──────────────────────────────────────────────────────────────┐
│                                                          │
│                    ┌─────────────────┐                          │
│                    │  MAIN (main.c)    │                          │
│                    └─────────────────┘                          │
│                                   ▲                │
│                    ┌─────────────────┐ ▼                │
│                    │   BUS (bus.c)   │ ▼                │
│                    └─────────────────┘ ▼                │
│                               ▲│ │                    ▼                    │
│        ┌─────────────┐ ▲│ │                    ▼                    │
│        │  CPU (cpu.c)   │ ▲│ │                    ▼                    │
│        └─────────────┘ ▲│ │  ┌──────────────┐│
│                         ▲   │  │  │ PPU (ppu.c)  ▲││
│    ┌──────────────────────┐ ▲   │  └──────────────┘ │
│    │   APU (apu.c)      │ ▲   └───────────────┘ │
│    └──────────────────────┘ ▲                         │
│                               ▲                    ▼
│        ┌─────────────┐ ▲│   ┌──────────────┐│
│        │  Mapper      │ ▲│   │ Cartridge  │ ▲│
│        │ (mapper.c)   │ ▲│   │ (rom.c)    │ ▲│
│        └─────────────┘ ▲│   └──────────────┘ │
│                         ▲                        │
│                    ┌─────────────┐ ▼                        │
│                    │  Input       │ ▼                        │
│                    │ (input.c)    │ ▼                        │
│                    └─────────────┘ ▼                        │
│                               ▲                    ▼                    │
└────────────────────────────────┴──────────────────────────────────────────────┘
```

### Data Flow

```
              ┌────────────────────┐
              │   main.c       │
              └────────────────────┘
                            │ load ROM → nes_sys_load_rom()
                            │              │
                            ▼                │
                            │   initialize    │   ┌─────────────────┐
                            │                │   │  Memory/Bus   │
                            │                │   └─────────────────┘
                            │                │              │
                            ▼                │              │
                            │  For each module (CPU, PPU, APU, Input):
                            │    nes_<module>_init() → nes_<module>_reset()
                            │    nes_<module>_step()     → execute one cycle
                            │    nes_<module>_cpu_read() / write() → access data
                            │                              │
                            │  Memory access flows through unified bus:      │
                            │  cpu_read → cartridge/mapper → prg rom      │
                            │  cpu_write → prg ram / mapper registers      │
                            │  cpu -> ppu registers via bus             │
                            │  ppu -> chr rom via                           │
                            │                                                      │
                            ▼                │              │
                            │   Frame loop:                                     │
                            │    nes_sys_step_frame():                              │
                            │    - CPU executes ~29780 cycles                  │
                            │    - PPU executes ~89341 cycles               │
                            │                                                │
                            │   Audio: nes_apu_generate_samples() → output   │
                            │   Video: nes_ppu_render_frame() → display        │
```

---

## Key Files & What They Do

### Core Emulation

| File | Description | Key Functions | Notes |
|------|-------------|------------------|-------|
| `src/cpu/cpu.c` | 6502 CPU emulation | `nes_cpu_step()`, `nes_cpu_read/write()`, all 151 opcodes |
| `src/ppu/ppu.c` | Video/emulation unit | `nes_ppu_step()`, scroll rendering, sprites, mirroring |
| `src/apu/apu.c` | audio processing | `nes_apu_generate_samples()`, pulse/triangle/noise/DMC channels |
| `src/mapper/mapper.c` | cart mapping | `mapper_N_init()`, CHR/PRG switching, specific mapper logic |

### Cartridge & Storage

| File | Description | Key Structures |
|------|-------------|-----------------|
| `src/cartridge/rom.c` | ROM loader | `nes_cartridge_load()`, parser for iNES format, CRC32 |
| `src/mapper/mapper.c` | mapper logic | `mapper_0_init()` through `mapper_7_init()` |

### System Integration

| File | Description | Key Functions |
|------|-------------|------------------|
| `src/memory/bus.c` | hardware integration | `nes_sys_init()`, `nes_sys_step_frame()`, unified read/write |
| `src/input/input.c` | controller I/O | NES controller port handling, button-to-keyboard mapping |

### Platform Layer

| File | Description | Platform Functions |
|------|-------------|-----------------|
| `src/platform/platform.c` | SDL2 integration | `nes_platform_init()`, window creation, event handling |

---

## Common Patterns & Conventions

### Naming Conventions

- **Functions**: `nes_<module>_<action>()` (e.g., `nes_cpu_reset()`, `nes_ppu_render_frame()`)
- **Variables**: Lower case with underscore (e.g., `cpu->reg.a`, `ppu->reg.status`)
- **Constants**: UPPER_CASE with underscore (e.g., `FLAG_CARRY`, `PPU_STATUS_VBLANK`)
- **Types**: `nes_<module>_t` for structs, `typedef enum` for enums
- **Global variables**: `g_<name>` prefix (e.g., `g_opcode_table`, `g_nes_palette`)

### Important Conventions

1. **All state must be in `nes_<module>_t` structs** - no direct struct access to individual fields
2. **Use typedef enums for fixed sets** (flags, opcodes, mappers)
3. **Pointer types use `void* context`** for callback context
4. **All memory access goes through the system bus**
5. **Use bit masks and bit operations** for flags and bitwise operations

### Memory Addressing

```
CPU Address Space (0x0000 - 0xFFFF):
├─ 0x0000-0x07FF   Internal RAM (2KB, mirrored)
├─ 0x0800-0x1FFF   PRG-ROM (switchable via mappers)
├─ 0x2000-0x3FFF   PPU registers
├─ 0x4000-0x4017   APU registers and I/O
└─ 0x4020-0x7FFF   Cartridge space

PPU Address Space (0x0000 - 0x7FFF):
├─ 0x0000-0x1FFF   CHR-ROM/CHR-RAM (8KB, switchable via mappers)
├─ 0x2000-0x3FFF   Palettes
└─ 0x4000-0x7FFF   VRAM/Nametables
```

### Mapper Implementation Pattern
```
Each mapper (0, 1, 2, 3, 4, 7) follows this pattern:

1. static <mapper>_ctx_t ctx;  // Holds cartridge pointer and state
2. static read/write functions using ctx
3. int <mapper>_init(nes_cartridge_t* cart, nes_mapper_t* mapper, [nes_ppu_t* ppu]):
   - Sets up ctx.cart = cart
   - Sets mapper number, callbacks, context
   - Returns 0 on success, -1 on unsupported
```

### Clock Synchronization

The emulator uses a 3:1 CPU:PPU cycle ratio:
- 1 PPU cycle = 3 CPU cycles
- Frame = 29,780 CPU cycles = 89,341 PPU cycles = 60 frames

---

## Adding Features

### Adding a New Mapper

Each mapper needs:
1. Static context struct with `nes_cartridge_t* cart` member
2. `cpu_read` wrapper function that calls `nes_sys_cpu_read(addr)`
3. `cpu_write` wrapper that calls `nes_sys_cpu_write(addr, val)`
4. `ppu_read/ch` wrapper for CHR-ROM access
5. `ppu_write/ch` for CHR-RAM
6. Function entry in `nes_mapper_create()` switch statement
7. Add enum value to NES_ROM_MAPPERS (or expand existing enum and support dynamic mapper detection)

Example pattern from mapper 0:
```c
typedef struct {
    nes_cartridge_t* cart;
} mapper_8_ctx_t;

static uint8_t mapper_8_cpu_read(void* ctx, uint16_t addr) {
    mapper_8_ctx_t* m = (mapper_8_ctx_t*)ctx;
    if (addr >= 0x6000 && addr < 0x8000 && m->cart->prg_ram)
        return m->cart->prg_ram[addr & 0x1FFF];
    return nes_cartridge_read_prg(m->cart, addr);
}
```

### Adding a New Opcode

When adding a new opcode:
1. Add to `g_opcode_table` with mnemonic, mode, cycles, length
2. Implement addressing mode handler (get_effective_address, load_byte/store_byte helpers)
3. Add case in the big switch in `nes_cpu_step()`
4. Update documentation in `MINGW_BUILD.md` and `README.md`

### Debugging CPU Issues

If CPU behavior is wrong:
1. Check `g_opcode_table` entries (mnemonic, mode, cycles, length, length)
2. Check addressing mode handlers (get_effective_address)
3. Test with known test ROMs (nestest, blargg, etc.)
4. Use `nes_cpu_disassemble()` to view what the CPU is actually doing

### Adding PPU Features

For scanline rendering issues:
1. Study `nes_ppu_step()` timing carefully
2. Each cycle (0-340 affects different PPU operations:
   - 0, 240: visible scanlines
   - 241: post-render
   - 242-260: vblank
   - 261: pre-render
3. Mirror modes affect nametable addressing
4. Sprites: 8 entries, 8x8 or 8x16
5. Sprite-0 hit detection when overlap occurs on non-transparent pixel

### Adding Audio Features

APU channels follow this architecture:
- **Pulse (2 channels)**: square waves with duty cycles, envelopes, sweep
- **Triangle**: linear counter sequenced triangle wave
- **Noise**: linear feedback shift register
- **DMC**: delta modulation channel with DMA from CPU memory

Each channel: 4 phase sequencer 15 steps long

---

## Known Issues & Gotchas

### Stack Bus Dependency Issue ⚠️

**Problem**: The `cpu_bus_t` struct holds function pointers that are copied from a stack-allocated struct in `nes_sys_init()`, causing crashes when the function returns.

**Solution**: Use separate static context storage instead of storing function pointers from the stack.

**Fix location**: `src/cpu/cpu.c` - use `g_cpu_bus_static` that contains only the context pointer, not function pointers.

### Mapper Static Context Issue ⚠️

**Problem**: Each mapper_init() uses static `ctx` variables that can overlap if multiple mappers are tried during initialization.

**Current workaround**: Only one mapper is ever created in `nes_mapper_create()`, but the static contexts share memory.

**Potential issue**: If mappers were dynamically loaded, static contexts would become problematic.

### Debug Output

When the emulator seg faults, add debug prints around:
1. `nes_sys_load_rom()` - shows mapper number, whether mapper creation succeeded
2. `nes_sys_reset()` - shows which module reset is being called and component pointers
3. `nes_ppu_step()` - shows scanline/cycle when crashes occur

### Memory Layout

```
PRG-ROM (switchable by mapper)   CHR-ROM/CHR-RAM (switchable or fixed)
├─ $8000-$BFFF   Switchable 16KB banks      └─ $0000-$1FFF   Fixed 8KB CHR (mapper 0) or $0000-$1FFF 8KB CHR (other mappers)
├─ $C000-$DFFF   Fixed PRG-ROM                      └─ $2000-$3FFF   Switchable 4KB PRG-ROM
└─ $E000-$FFFF   Fixed PRG-ROM (last bank)

VRAM (switchable by mirroring):
- Horizontal: [0][1] [2][3] ← → [0][1][2][3]
- Vertical:   [0][2] [1][3] ← → [0][2][1][3]
- Single 0 mode: [0][0][0][0][0] ← → [0][0][0][0][0]
- Single 1 mode: [1][1][1][1] ← → [1][1][1][1]
```

### Palette Order

NES palette (0x00-0x3F):
- 0x00-0x0F: Background colors
- 0x10-0x1F: Sprite colors
- 0x20-0x2F: Background colors
- 0x30-0x3F: Sprite colors

Each color is a palette index that points into the 64 color palette.

---

## Important Locations

### Core Emulation
- `src/cpu/cpu.h` - CPU interface and constants
- `src/cpu/cpu.c` - CPU implementation and opcodes
- `src/ppu/ppu.h` - PPU interface and constants
- `src/ppu/ppu.c` - PPU implementation
- `src/apu/apu.h` - APU interface and constants
- `src/apu/apu.c` - APU implementation

### System Integration
- `src/memory/bus.h` - system interface, memory layout, API
- `src/memory/bus.c` - system implementation, bus implementation
- `src/main.c` - main program, SDL2 event loop

### Cartridge System
- `src/cartridge/rom.h` - ROM data structures, parsing
- `src/cartridge/rom.c` - ROM loading, CRC32, SRAM handling
- `src/mapper/mapper.h` - mapper interface, creation function
- `src/mapper/mapper.c` - mapper implementations

### Input & Testing
- `src/input/input.h` - controller interface
- `src/input/input.c` - controller implementation
- `src/input/input.c` - controller implementation

---

## Testing on Hardware

Use test ROMs from https://www.nesdev.org/wiki/Emulator_tests

**CPU Tests:**
- cpu.nes (basic CPU tests)
- instr_test_v2 (instruction tests)
- timing.nes (branch tests)

**PPU Tests:**
- ppu_vblank.nes (VBlank timing)
- sprite_hit.nes (sprite-0 hit detection)
- timing.nes (PPU timing)

**Integration Tests:**
- nestest.nes (comprehensive emulator tests)
- official test ROMs

---

## Version History

Key commits that changed behavior:
- Initial commit: basic emulator structure
- "🔧 Compilation error fixes" - fixed compilation errors
- "🐛 Add debug output" - added tracing output
- "🐛 Fix segfault in nes_cpu_reset" - NULL check added

---

When making changes that break existing ROM compatibility, note the commit in this file.
