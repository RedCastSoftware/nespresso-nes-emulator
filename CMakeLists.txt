cmake_minimum_required(VERSION 3.15)
project(NESPRESSO VERSION 0.9.0 LANGUAGES C)

# ================================
# NESPRESSO - NES Emulator
# Brewing Nostalgia One Frame at a Time!
# ================================

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

# Platform-specific settings
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /W4 /D_CRT_SECURE_NO_WARNINGS")
    # Use static runtime for easier distribution
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native")
endif()

# Find SDL2
if(WIN32)
    # On Windows, SDL2 can be in various locations
    # Try vcpkg first, then system, then local
    find_package(SDL2 CONFIG QUIET)
    if(NOT SDL2_FOUND)
        find_package(SDL2 REQUIRED)
    endif()
else()
    find_package(SDL2 REQUIRED)
endif()

# Include directories
include_directories(
    ${SDL2_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

# Source files
set(SOURCES
    src/main.c
    src/cpu/cpu.c
    src/ppu/ppu.c
    src/apu/apu.c
    src/cartridge/rom.c
    src/mapper/mapper.c
    src/input/input.c
    src/memory/bus.c
    src/platform/platform.c
)

# Create executable
add_executable(NESPRESSO ${SOURCES})

# Link libraries
target_link_libraries(NESPRESSO PRIVATE
    SDL2::SDL2
    SDL2::SDL2main
)

# Installation
install(TARGETS NESPRESSO
    RUNTIME DESTINATION bin
)

# Copy SDL2 DLLs on Windows (for standalone build)
if(WIN32)
    # Find SDL2 DLL location
    get_target_property(SDL2_DLL SDL2::SDL2 IMPORTED_IMPLIB)
    get_filename_component(SDL2_DLL_DIR ${SDL2_DLL} DIRECTORY)

    # Try to find the DLL
    find_file(SDL2_DLL_PATH
        NAMES SDL2.dll SDL2d.dll
        PATHS ${SDL2_DLL_DIR}
        NO_DEFAULT_PATH
    )

    if(SDL2_DLL_PATH)
        install(FILES ${SDL2_DLL_PATH} DESTINATION bin)
    endif()
endif()

# Platform-specific options
option(ENABLE_AUDIO "Enable audio output" ON)
option(ENABLE_DEBUG "Enable debug build" OFF)

if(ENABLE_DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

# Print configuration summary
message(STATUS "")
message(STATUS "NESPRESSO Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  SDL2 Found: ${SDL2_FOUND}")
message(STATUS "  SDL2 Include: ${SDL2_INCLUDE_DIRS}")
message(STATUS "  Compiler: ${CMAKE_C_COMPILER_ID} ${CMAKE_C_COMPILER_VERSION}")
message(STATUS "")
