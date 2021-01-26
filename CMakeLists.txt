# Set the CMake version
cmake_minimum_required(VERSION 3.18)

# Create the project
project(Chip8 LANGUAGES C)

# Add the main executable
add_executable(Chip8
    "src/main.c"
    "src/emulator.h"
    "src/emulator.c"
    "src/chip8.h"
    "src/chip8.c"
    "src/screen.h"
    "src/screen.c"
    "src/buzzer.h"
    "src/buzzer.c"
)

# Add SDL2 to the list of CMake modules
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl2)

# Add SDL2 library
find_package(SDL2 REQUIRED)
target_link_libraries(Chip8 SDL2::Main)
