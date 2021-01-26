#ifndef EMULATOR_H
#define EMULATOR_H

#include <SDL.h>

#include <stdint.h>
#include <stdbool.h>

#include "chip8.h"
#include "screen.h"
#include "buzzer.h"

typedef struct Emulator {
    // The chip component performing the real processing - executing programs
    // and writing graphics data.
    Chip8 chip;
    
    // The presentation component for the emulation. The screen is responsible
    // for displaying the emulator on the window.
    Screen screen;
    
    // The audio component. Because CHIP-8 doesn't have sophisticated audio output,
    // it simply needs to make a buzzing sound.
    Buzzer buzzer;
    
    // Flag controlling the main loop of the emulator.
    bool running;
    
    // Flag controlling whether the chip emulation is currently on pause.
    bool paused;
    
    // Clock frequency (Hz) that controls the amount of cycles (instructions)
    // the chip executes per second.
    double clock_freq;
    
    // The amount of time (ns) that needs to pass before the chip, timers,
    // and screen get updated.
    Uint64 clock_period, timer_period, refresh_period;
    
    // Pointer to the current ROM, if loaded from file, for later deallocation.
    uint8_t *rom;
    
    // The window bound to this emulator.
    SDL_Window *window;
} Emulator;

bool Emulator_Init(Emulator *emulator);
void Emulator_Free(Emulator *emulator);

// Helper functions for retrieving various frequencies from the emulator
double Emulator_ClockFreq(Emulator *emulator);
double Emulator_TimerFreq(Emulator *emulator);
double Emulator_RefreshFreq(Emulator *emulator);

// Pre-loads user-specified ROM into the emulator on start. This function must be
// called before `Emulator_Run`. If `path` is NULL, loads a default ROM.
bool Emulator_Preload(Emulator *emulator, const char *path);

// Starts the emulator, which will keep running until explicitly shut down
// by the user.
void Emulator_Run(Emulator *emulator);

#endif