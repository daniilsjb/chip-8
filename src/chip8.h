#ifndef CHIP8_H
#define CHIP8_H

#include <stdint.h>
#include <stdbool.h>

// The total size of CHIP-8 RAM, in bytes.
#define RAM_SIZE 4096

// The number of general-purpose registers (labeled V0 - VF)
#define NUM_REGISTERS 16

// The size of the subroutine stack
#define NUM_STACK 16

// The number of keys on the chip's hexpad
#define NUM_KEYS 16

// Native video display dimensions
#define VIDEO_WIDTH 64
#define VIDEO_HEIGHT 32

typedef struct Chip8 {
    // General-purpose data registers.
    uint8_t V[NUM_REGISTERS];
    
    // Address register.
    uint16_t I;
    
    // Delay register used for timing. Ticks down at a constant frequency
    // until it reaches 0.
    uint8_t delay;
    
    // Sound register used for sound. Ticks down at a constant frequency
    // until it reaches 0, producing a buzzing sound on each tick.
    uint8_t sound;
    
    // Program counter pointing to the address in RAM of the next instruction
    // to execute.
    uint16_t pc;
    
    // Stack pointer for the subroutine currently being executed.
    uint8_t sp;
    
    // Subroutine stack. Stores addresses of the subroutine callers.
    uint16_t stack[NUM_STACK];
    
    // Random-access memory of the chip, 4KB. The lower 512 bytes are reserved.
    uint8_t *ram;
    
    // Video memory containing graphics data. Consists of 64x32 bits, where each bit
    // maps to a single pixel on the monochrome display. Pixels are mapped in MSB
    // order, i.e. the MSB of a given row corresponds to x of 0, and the LSB to x of 63.
    uint64_t video[VIDEO_HEIGHT];
    
    // Hexpad data. For each key stores a flag which represents whether it is currently
    // pressed.
    bool keys[NUM_KEYS];
    
    // If the chip is waiting for a key press, this field contains the number of the
    // register where the input is to be stored. Otherwise contains an invalid register
    // number.
    uint8_t key_reg;
} Chip8;

bool Chip8_Init(Chip8 *chip);
void Chip8_Free(Chip8 *chip);

// Restarts execution of the currently loaded ROM. This function sets all registers
// to their default values and clears the video memory.
void Chip8_Restart(Chip8 *chip);

// Loads a new program into the program space of RAM.
void Chip8_LoadProgram(Chip8 *chip, uint8_t *program, size_t size);

// Clears the program space of RAM.
void Chip8_ClearProgram(Chip8 *chip);

// Resets the chip. Shortcut for `Chip8_Restart` and `Chip8_ClearProgram`.
void Chip8_Reset(Chip8 *chip);

// Updates the state of a key corresponding to a digit on the hexpad.
void Chip8_UpdateKey(Chip8 *chip, uint8_t digit, bool pressed);

// Updates the timer registers.
void Chip8_UpdateTimers(Chip8 *chip);

// Runs a single cycle of execution.
void Chip8_Step(Chip8 *chip);

#endif