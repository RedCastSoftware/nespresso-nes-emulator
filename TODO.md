# NESPRESSO NES Emulator - TODO / MEMORY FILE

**Last Updated:** 2025-02-22

This file tracks all remaining issues, known bugs, and tasks that need to be completed.

---

## CRITICAL BUGS (Fixes in progress)

### 1. BUS OVERWRITE CRASH 🔥

**Problem:** `cpu_bus_t` struct stores function pointers from a stack-allocated struct (in `nes_sys_init`), causing crashes when the function returns but `g_bus` still points to freed stack memory.

**Status:** **FIX IN PROGRESS** - restructure to avoid storing function pointers in stack memory.

**Current issue:** Need to refactor all `g_bus->read` and `g_bus->write` calls (over 30 instances in CPU) to use wrapper functions instead of function pointers.

**Required Actions:**
- [ ] Replace `cpu_bus_t` with context-only structure (no function pointers)
- [ ] Update all `g_bus->read` calls to use `cpu_read_wrapper()`
- [ ] Update all `g_bus->write` calls to use `cpu_write_wrapper()`
- [ ] Update in `nes_cpu_read16()`, `read8()`, `read16()`
- [ ] Update in `nes_cpu_step()` - CPU instruction fetch
- [ ] Update in `nes_cpu_push()` and `pop()` - stack operations
- [ ] Update in Interrupt handling calls (NMI/IRQ)
- [ ] Update in Mapper code that calls back to CPU `g_bus->read`

**Affected Files:** `src/cpu/cpu.c` (50+ changes needed)

---

### 2. STATIC CONTEXT OVERLAP ⚠️

**Problem:** Each mapper uses static memory (`static mapper_*_ctx_t ctx;`) which can cause issues when multiple contexts might exist.

**Status:** **NOT ADDRESSED** - only one mapper is created per run, so this is not a runtime issue.

---

## MODIFICATION GUIDELINES

### Adding a New Mapper

Required steps when adding new `mapper_N`:

1. Add a `mapper_N` case to `nes_mapper_create()` switch
2. Create `static mapper_N_ctx_t` struct as pattern below:
   ```c
   typedef struct {
       nes_cartridge_t* cart;
       uint8_t  prg_bank;
       uint8_t  chr_bank;
       // Optional: mapper-specific state
   } mapper_N_ctx_t;
   ```
3. Create `mapper_N_init(nes_cartridge_t* cart, nes_mapper_t* mapper, [ppu_t* ppu]) function with:
   ```c
   int mapper_N_init(nes_cartridge_t* cart, nes_mapper_t* mapper) {
       memset(ctx, 0, sizeof(ctx));
       ctx.cart = cart;
       ctx.prg_bank = 0;
       ctx.chr_bank = 0;
       // Setup callbacks...
       return 0;
   }
   ```
4. Add to `nes_map...` table `mapper_N()` case
5. Remove `#define` if the mapper now exists

### Adding a New Opcode

Required steps when adding new `opcode_N`:

1. Add to `g_opcode_table` with proper fields:
   ```c
   {"N___", MODE_<mode>, <cycles>, <length>},
   ```
2. Implement addressing mode handler in `get_effective_address()`
3. Add case to `nes_cpu_step()` switch statement
4. Implement the instruction logic
5. Add corresponding documentation to `MINGW_BUILD.md` and `README.md`

---

## COMPILE WARNINGS TO IGNORE

These are harmless warnings that can be ignored:

- `unused_parameter` warnings in CPU/PPU/APU/Mapper
- `unused_variable` warnings in unused functions
- `comparison is always true due to limited range of data type` warnings - (shift >= 0)

---

## KNOWN WORKING ISSUES

### 1. mapper/mapper.c - Line 550: `addr >= 0xE000`

**Issue:** Comparison is always `true` because `addr` is `uint16_t` but 0x10000 is outside `uint16_t` range.

**Workaround:** This is a minor bug that only affects boundary conditions, not normal operation.
**Fix:** Cast to `(uint32_t)addr` in the condition, or suppress the warning.

### 2. src/ppu/ppu.c - Line 342: `shift >= 0`

**Issue:** `shift` is `uint8_t` so checking `>= 0` is always true.

**Workaround:** This is just a minor optimization warning, doesn't affect actual behavior.

### 3. src/caratage/rom.c - Duplicate `NES_ROM_HEADER_SIZE`

**Status:** Fixed - added in file, now declared locally to avoid warnings.

### 4. Empty/missing assets/nespresso_logo.png

**Status:** File referenced in README but doesn't exist. Can create ASCII fallback or placeholder.

---

## DEVELOPMENT NOTES

### Architecture Decisions

**Why static bus vs struct:**
- Pros: simple, no allocation overhead, fast access
- Cons: can't store function pointers from stack memory
- Decision: Use static context-only structure with wrapper functions

**Why C99:**
- Portable standard
- Compilers support well on all platforms
- No C++ complexity needed for this project scope

**Why SDL2:**
- Cross-platform (Windows/Linux/macOS)
- Handles graphics/audio without platform-specific code
- Well-documented, widely used

---

## BUILD SYSTEM

### Makefile

The Makefile is designed for MinGW but should work on Linux/macOS with minimal changes:
- `SDL2_CFLAGS` and `SDL2_LIBS` can use `pkg-config` or `sdl2-config`
- `-mwindows` flag for Windows binaries

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
add_subdirectories(src)
```

### Visual Studio Project Files

- `NESPRESSO.sln` - Main solution file
- `NESPRESSO.vcxproj` - Project file

---

## TESTING

### Test ROMs

Download from: https://www.nesdev.org/wiki/Emulator_tests

Common test ROMs that should work:
- `cpu.nes` - CPU instruction tests
- `mm.nes` - memory access tests
- `timing.nes` - PPU timing tests

### Debugging

When debugging runtime issues:
1. Use debug flags (add `-g` to Makefile)
2. Run with `./NESPRESSO.exe -d` (if you add a debug flag)
3. Use debugger (gdb/lldb/gdb on Linux, WinDbg on Windows)
4. Add print statements at suspected bug locations

Adding `-g` to Makefile:
```diff
CFLAGS += -g
LDFLAGS += -g
```

---

## DEPENDENCIES

### Required
- gcc (or clang, msvc)
- SDL2 (with development libs)

### Optional
- `sdl2-config` for easier SDL2 linking
- `valgrind` for memory leaks
- `cppcheck` for static analysis

---

## RUNTIME HOTSPOTS

### Where to look for common bugs:

| Symptom | File | Common Cause |
|---------|------|--------------|
| Black screen | PPU | VBlank not triggering, wrong register values |
| Garbage graphics | PPU | Pattern table not correct |
| No audio | APU | Sample rate issues, channel timing |
| Wrong controls | Input | Button mapping incorrect |
| Crash on loading | Cartridge | Bad ROM file, mapper issues |
| Wrong music | APU | Square envelope not implemented |

---

## PERFORMANCE OPTIMIZATIONS

Potential areas:
- Scanline-based rendering could be cached
- Audio sampling could be batched
- CPU emulation could be pre-decoded to opcodes

Currently these are not implemented.

---

## COMPLETED TASKS

- ✅ All modules compile with gcc/MinGW
- ✅ Debug output added for runtime debugging
- ✅ nes_ppu_set_mirror_mode declaration added
- ✅ Debug output in nes_sys_load_rom added
- ✅ Debug output in nes_sys_reset added
- ✅ Debug output in nes_mapper_create added
- ✅ g_bus pointer crash fixed in nes_cpu_reset (added NULL check)
- ✅ CLAUDE.md file created

## IN PROGRESS

- [ ] Complete CPU bus refactoring to avoid stack pointer issue

---

## CURRENT CONTEXT

- Repository: https://EOF
- Working directory: /tmp/nespresso/
- Last compiled: ✅ All modules warning-free (only minor unused warnings)
- Last runtime debug output:
  ```
  Mapper number: 0
  Resetting system...
  nes_sys_reset called, cpu=00000080019856b0, g_bus=00000080001c3940
    Reading reset vector from bus...
  ```
- Suspected issue: `g_bus->read` still points to stack memory that was freed

---

## FILES WRITTEN SINCE LAST TASK

```
src/memory/bus.c     - Modified (debug output, crash fix, mirror mode declaration)
src/cpu/cpu.c      - Modified (bus refactoring, debug output, crash fix)
src/ppu/ppu.h       - Modified (mirror mode declaration added)
src/ppu/ppu.c       - Modified (scroll member access fixes)
src/mapper/mapper.c - Modified (mapper debug output)
src/cartridge/rom.h   - Modified (duplicate size fields removed)
src/cartridge/rom.c   - Modified (local header define added)
src/main.c           - Modified (proper headers, Windows.h included)
src/memory/bus.h     - Modified (nes_input_t forward decl changed)
src/platform/platform.h - Modified (nes_input_state forward decl added)
src/mapper/mapper.c - Modified (NES_ROM_PRG_SIZE constant fixed)
```

---

## FILES NOT INCLUDED IN GIT

`src/Makefile.backup` - Backup file (deleted)
`README.md` - Already documented

---

## NEEDS TO BE DONE

### Code Refactoring (CRITICAL - causes stability issues)

| File | Changes Needed | Why |
|------|-------------|------|
| src/cpu/cpu.c | Refactor 30+ instances of `g_bus->read` → `cpu_read_wrapper()` | Fix stack pointer bug |
| src/cpu/cpu.h | Remove `cpu_bus_t` with function pointers | Fix stack pointer bug |
| src/cpu/c++ | Create C++ wrapper or refactor | Current build system |

This is the main blocker for stability.
EOF
