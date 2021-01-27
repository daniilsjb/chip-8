#include "chip8.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Offset of the program space within RAM. The first 512 bytes are
// reserved for the interpreter itself, which is not necessary
// for an emulator. This allows us to store other data in there,
// such as the font.
#define PROGRAM_START 512

static bool InitMemory(Chip8 *chip);
static bool InitFont(Chip8 *chip);

bool Chip8_Init(Chip8 *chip)
{
    memset(chip, 0, sizeof(*chip));
    
    if (!InitMemory(chip)) {
        return false;
    }
    if (!InitFont(chip)) {
        return false;
    }
    
    Chip8_Reset(chip);
    return true;
}

bool InitMemory(Chip8 *chip)
{
    if (!(chip->ram = malloc(RAM_SIZE))) {
        return false;
    }
    
    memset(chip->ram, 0, RAM_SIZE);
    return true;
}

bool InitFont(Chip8 *chip)
{
    // This font is directly taken from the CHIP-8 specification
    static const uint8_t font[] = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, /* '0' */
        0x20, 0x60, 0x20, 0x20, 0x70, /* '1' */
        0xF0, 0x10, 0xF0, 0x80, 0xF0, /* '2' */
        0xF0, 0x10, 0xF0, 0x10, 0xF0, /* '3' */
        0x90, 0x90, 0xF0, 0x10, 0x10, /* '4' */
        0xF0, 0x80, 0xF0, 0x10, 0xF0, /* '5' */
        0xF0, 0x80, 0xF0, 0x90, 0xF0, /* '6' */
        0xF0, 0x10, 0x20, 0x40, 0x40, /* '7' */
        0xF0, 0x90, 0xF0, 0x90, 0xF0, /* '8' */
        0xF0, 0x90, 0xF0, 0x10, 0xF0, /* '9' */
        0xF0, 0x90, 0xF0, 0x90, 0x90, /* 'A' */
        0xE0, 0x90, 0xE0, 0x90, 0xE0, /* 'B' */
        0xF0, 0x80, 0x80, 0x80, 0xF0, /* 'C' */
        0xE0, 0x90, 0x90, 0x90, 0xE0, /* 'D' */
        0xF0, 0x80, 0xF0, 0x80, 0xF0, /* 'E' */
        0xF0, 0x80, 0xF0, 0x80, 0x80  /* 'F' */
    };
    
    memcpy(chip->ram, font, sizeof(font) / sizeof(font[0]));
    return true;
    
}

void Chip8_Free(Chip8 *chip)
{
    if (chip == NULL) {
        return;
    }
    
    free(chip->ram);
}

static void ClearScreen(Chip8 *chip)
{
    memset(chip->video, 0, sizeof(uint64_t) * VIDEO_HEIGHT);
}

void Chip8_Restart(Chip8 *chip)
{
    // Clear arrays and the screen
    memset(chip->V, 0, NUM_REGISTERS);
    memset(chip->stack, 0, NUM_STACK);
    
    ClearScreen(chip);

    // Clear timer registers
    chip->delay = 0;
    chip->sound = 0;

    // Clear instruction registers
    chip->pc = PROGRAM_START;
    chip->sp = 0;
    chip->I = 0;
    
    // Clear the key register, as we're not waiting for input anymore
    chip->key_reg = 0xFF;
}

void Chip8_LoadProgram(Chip8 *chip, uint8_t *program, size_t size)
{
    // Put the program data right after the reserved memory
    memcpy(chip->ram + PROGRAM_START, program, size);
}

void Chip8_ClearProgram(Chip8 *chip)
{
    // Clear everything after the reserved memory
    memset(chip->ram + PROGRAM_START, 0, RAM_SIZE - PROGRAM_START);
}

void Chip8_Reset(Chip8 *chip)
{
    // Fully reset the variant chip state
    Chip8_Restart(chip);
    Chip8_ClearProgram(chip);
}

void Chip8_UpdateKey(Chip8 *chip, uint8_t digit, bool pressed)
{
    // Set the key status
    chip->keys[digit] = pressed;
    
    // If waiting for input and the key has been pressed, write it into
    // the specified register
    if (chip->key_reg < NUM_REGISTERS && pressed) {
        chip->V[chip->key_reg] = digit;
        chip->key_reg = 0xFF;
    }
}

void Chip8_UpdateTimers(Chip8 *chip)
{
    // Timers should simply tick down until they reach 0
    if (chip->delay > 0) {
        chip->delay--;
    }
    if (chip->sound > 0) {
        chip->sound--;
    }
}

// Helper macros for retrieving parts of an instruction
#define NNN(i) (((i) & 0x0FFF) >> 0)
#define NN(i)  (((i) & 0x00FF) >> 0)
#define N(i)   (((i) & 0x000F) >> 0)
#define X(i)   (((i) & 0x0F00) >> 8)
#define Y(i)   (((i) & 0x00F0) >> 4)

// Forward declarations of every opcode's function. Opcode names are original,
// but an attempt was made to be authentic to assembly mnemonics.
static void cls(Chip8 *chip, uint16_t i);
static void ret(Chip8 *chip, uint16_t i);
static void jmp(Chip8 *chip, uint16_t i);
static void call(Chip8 *chip, uint16_t i);
static void eq(Chip8 *chip, uint16_t i);
static void neq(Chip8 *chip, uint16_t i);
static void eqv(Chip8 *chip, uint16_t i);
static void neqv(Chip8 *chip, uint16_t i);
static void ldn(Chip8 *chip, uint16_t i);
static void addn(Chip8 *chip, uint16_t i);
static void mov(Chip8 *chip, uint16_t i);
static void or(Chip8 *chip, uint16_t i);
static void and(Chip8 *chip, uint16_t i);
static void xor(Chip8 *chip, uint16_t i);
static void addv(Chip8 *chip, uint16_t i);
static void subl(Chip8 *chip, uint16_t i);
static void subr(Chip8 *chip, uint16_t i);
static void lsh(Chip8 *chip, uint16_t i);
static void rsh(Chip8 *chip, uint16_t i);
static void ldi(Chip8 *chip, uint16_t i);
static void jmpn(Chip8 *chip, uint16_t i);
static void rnd(Chip8 *chip, uint16_t i);
static void draw(Chip8 *chip, uint16_t i);
static void key(Chip8 *chip, uint16_t i);
static void nkey(Chip8 *chip, uint16_t i);
static void dly(Chip8 *chip, uint16_t i);
static void ldd(Chip8 *chip, uint16_t i);
static void wait(Chip8 *chip, uint16_t i);
static void snd(Chip8 *chip, uint16_t i);
static void addi(Chip8 *chip, uint16_t i);
static void dgt(Chip8 *chip, uint16_t i);
static void bcd(Chip8 *chip, uint16_t i);
static void load(Chip8 *chip, uint16_t i);
static void fill(Chip8 *chip, uint16_t i);

void Chip8_Step(Chip8 *chip)
{
    // If key register is set, the chip is waiting for input
    if (chip->key_reg < NUM_REGISTERS) {
        return;
    }
    
    // Fetch the next instruction. Instructions are stored in big-endian order and occupy two bytes.
    uint16_t i = 0;
    i |= chip->ram[chip->pc++] << 8;
    i |= chip->ram[chip->pc++] << 0;
    
    // Decode the instruction. To make things complex, opcodes are not just numbers - in CHIP-8
    // opcodes are represented as patterns within instructions. This means the same nibbles (4 bits)
    // may be used as arguments in some instructions and be part of opcodes in others.
    //
    // To decode an instruction, we go through each possible opcode and check if the pattern is
    // matched. We do so by first fully clearing the nibbles that should contain this opcode's
    // arguments, thus leaving only the opcode bits set. Then we directly check the resulting bits
    // to see if they match the opcode's. If no match was found, the chip skips this instruction.
    if (i == 0x00E0) {
        cls(chip, i);
    } else if (i == 0x00EE) {
        ret(chip, i);
    } else if ((i & 0xF000) == 0x1000) {
        jmp(chip, i);
    } else if ((i & 0xF000) == 0x2000) {
        call(chip, i);
    } else if ((i & 0xF000) == 0x3000) {
        eq(chip, i);
    } else if ((i & 0xF000) == 0x4000) {
        neq(chip, i);
    } else if ((i & 0xF00F) == 0x5000) {
        eqv(chip, i);
    } else if ((i & 0xF000) == 0x6000) {
        ldn(chip, i);
    } else if ((i & 0xF000) == 0x7000) {
        addn(chip, i);
    } else if ((i & 0xF00F) == 0x8000) {
        mov(chip, i);
    } else if ((i & 0xF00F) == 0x8001) {
        or(chip, i);
    } else if ((i & 0xF00F) == 0x8002) {
        and(chip, i);
    } else if ((i & 0xF00F) == 0x8003) {
        xor(chip, i);
    } else if ((i & 0xF00F) == 0x8004) {
        addv(chip, i);
    } else if ((i & 0xF00F) == 0x8005) {
        subl(chip, i);
    } else if ((i & 0xF00F) == 0x8006) {
        rsh(chip, i);
    } else if ((i & 0xF00F) == 0x8007) {
        subr(chip, i);
    } else if ((i & 0xF00F) == 0x800E) {
        lsh(chip, i);
    } else if ((i & 0xF00F) == 0x9000) {
        neqv(chip, i);
    } else if ((i & 0xF000) == 0xA000) {
        ldi(chip, i);
    } else if ((i & 0xF000) == 0xB000) {
        jmpn(chip, i);
    } else if ((i & 0xF000) == 0xC000) {
        rnd(chip, i);
    } else if ((i & 0xF000) == 0xD000) {
        draw(chip, i);
    } else if ((i & 0xF0FF) == 0xE09E) {
        key(chip, i);
    } else if ((i & 0xF0FF) == 0xE0A1) {
        nkey(chip, i);
    } else if ((i & 0xF0FF) == 0xF007) {
        ldd(chip, i);
    } else if ((i & 0xF0FF) == 0xF00A) {
        wait(chip, i);
    } else if ((i & 0xF0FF) == 0xF015) {
        dly(chip, i);
    } else if ((i & 0xF0FF) == 0xF018) {
        snd(chip, i);
    } else if ((i & 0xF0FF) == 0xF01E) {
        addi(chip, i);
    } else if ((i & 0xF0FF) == 0xF029) {
        dgt(chip, i);
    } else if ((i & 0xF0FF) == 0xF033) {
        bcd(chip, i);
    } else if ((i & 0xF0FF) == 0xF055) {
        load(chip, i);
    } else if ((i & 0xF0FF) == 0xF065) {
        fill(chip, i);
    }
}

// Clear the screen
void cls(Chip8 *chip, uint16_t i)
{
    ClearScreen(chip);
}

// Return from a subroutine
void ret(Chip8 *chip, uint16_t i)
{
    chip->pc = chip->stack[--chip->sp];
}

// Jump to address NNN
void jmp(Chip8 *chip, uint16_t i)
{         
    chip->pc = NNN(i);
}

// Execute subroutine starting at address NNN
void call(Chip8 *chip, uint16_t i)
{      
    chip->stack[chip->sp++] = chip->pc;
    chip->pc = NNN(i);
}

// Skip the following instruction if the value of register VX equals NN
void eq(Chip8 *chip, uint16_t i)
{
    if (chip->V[X(i)] == NN(i)) {
        chip->pc += 2;
    }
}

// Skip the following instruction if the value of register VX is not equal to NN
void neq(Chip8 *chip, uint16_t i)
{       
    if (chip->V[X(i)] != NN(i)) {
        chip->pc += 2;
    }
}

// Skip the following instruction if the value of register VX is equal to the value of register VY
void eqv(Chip8 *chip, uint16_t i)
{
    if (chip->V[X(i)] == chip->V[Y(i)]) {
        chip->pc += 2;
    }
}

// Skip the following instruction if the value of register VX is not equal to the value of register VY
void neqv(Chip8 *chip, uint16_t i)
{
    if (chip->V[X(i)] != chip->V[Y(i)]) {
        chip->pc += 2;
    }
}

// Store number NN in register VX
void ldn(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] = NN(i);
}

// Add the value NN to register VX
void addn(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] += NN(i);
}

// Store the value of register VY in register VX
void mov(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] = chip->V[Y(i)];
}

// Set VX to VX OR VY
void or(Chip8 *chip, uint16_t i)
{       
    chip->V[X(i)] |= chip->V[Y(i)];
}

// Set VX to VX AND VY
void and(Chip8 *chip, uint16_t i)
{         
    chip->V[X(i)] &= chip->V[Y(i)];
}

// Set VX to VX XOR VY
void xor(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] ^= chip->V[Y(i)];
}

// Add the value of register VY to register VX
// Set VF to 01 if a carry occurs
// Set VF to 00 if a carry does not occur
void addv(Chip8 *chip, uint16_t i)
{
    // To check for overflow, we explicitly store the result in a 16-bit number first,
    // and then see if it exceeded the range of a byte. Casting it to 8 bits should
    // then truncate it back.
    uint16_t sum = (uint16_t)chip->V[X(i)] + (uint16_t)chip->V[Y(i)];
    chip->V[0xF] = sum > 255;
    chip->V[X(i)] = (uint8_t)sum;
}

// Subtract the value of register VY from register VX
// Set VF to 00 if a borrow occurs
// Set VF to 01 if a borrow does not occur
void subl(Chip8 *chip, uint16_t i)
{
    // A borrow will only occur if the subtrahend is greater than the minuend
    chip->V[0xF] = chip->V[X(i)] >= chip->V[Y(i)];
    chip->V[X(i)] -= chip->V[Y(i)];
}

// Set register VX to the value of VY minus VX
// Set VF to 00 if a borrow occurs
// Set VF to 01 if a borrow does not occur
void subr(Chip8 *chip, uint16_t i)
{
    // A borrow will only occur if the subtrahend is greater than the minuend
    chip->V[0xF] = chip->V[Y(i)] >= chip->V[X(i)];
    chip->V[X(i)] = chip->V[Y(i)] - chip->V[X(i)];
}

// Store the value of register VY shifted left one bit in register VX
// Set register VF to the most significant bit prior to the shift
void lsh(Chip8 *chip, uint16_t i)
{
    // We check if the MSB is set, and then use double negation to transfrom
    // a non-zero into a one, as is expected by the chip
    chip->V[0xF] = !!(chip->V[Y(i)] & (1 << 7));
    chip->V[X(i)] = chip->V[Y(i)] << 1;
}

// Store the value of register VY shifted right one bit in register VX
// Set register VF to the least significant bit prior to the shift
void rsh(Chip8 *chip, uint16_t i)
{
    chip->V[0xF] = chip->V[Y(i)] & 1;
    chip->V[X(i)] = chip->V[Y(i)] >> 1;
}

// Store memory address NNN in register I
void ldi(Chip8 *chip, uint16_t i)
{
    chip->I = NNN(i);
}

// Jump to address NNN + V0
void jmpn(Chip8 *chip, uint16_t i)
{    
    chip->pc = NNN(i) + chip->V[0];
}

// Set VX to a random number with a mask of NN
void rnd(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] = rand() & NN(i);
}

// Draw a sprite at position VX, VY with N bytes of sprite data starting at the address stored in I
// Set VF to 01 if any set pixels are changed to unset, and 00 otherwise
void draw(Chip8 *chip, uint16_t i)
{
    // Retrieve the coordinates and number of bytes in the sprite
    uint8_t x = chip->V[X(i)];
    uint8_t y = chip->V[Y(i)];
    uint8_t h = N(i);
    
    // Assume there will be no XOR collision
    chip->V[0xF] = 0;
    
    // Find the position of the sprite within RAM
    uint8_t *sprite = chip->ram + chip->I;
    
    // Scan each row of the sprite
    for (uint8_t dy = 0; dy < h; dy++) {
        // Calculate the row in VRAM we we will be drawing to. We must ensure that it is within
        // the valid range. We do this by wrapping it back to the top in case it's too large.
        int vy = (y + dy) % VIDEO_HEIGHT;
        
        // Put the sprite data into correct position along the 64-bit row. We first put the
        // data into the most significant byte of the row, and then use the x coordinate to
        // determine its final position.
        uint64_t mask = ((uint64_t)sprite[dy] << (VIDEO_WIDTH - 8)) >> x;
        
        // A bit of a hack, but we know that a collision will only occur if there is at least
        // one pair of bits that is set in both the VRAM row and the sprite data. By AND'ing
        // them we get a non-zero value if there is at least one such pair. The double negation
        // trick allows us to transform any non-zero into a one. Then we set the flag if it's not
        // been set yet.
        chip->V[0xF] |= !!(chip->video[vy] & mask);
        
        // Finally, XOR the entire row with the mask to draw the sprite data.
        chip->video[vy] ^= mask;
    }
}

// Skip the following instruction if the key corresponding to the hex value currently stored in register VX is pressed
void key(Chip8 *chip, uint16_t i)
{
    if (chip->keys[chip->V[X(i)]]) {
        chip->pc += 2;
    }
}

// Skip the following instruction if the key corresponding to the hex value currently stored in register VX is not pressed
void nkey(Chip8 *chip, uint16_t i)
{
    if (!chip->keys[chip->V[X(i)]]) {
        chip->pc += 2;
    }
}

// Set the delay timer to the value of register VX
void dly(Chip8 *chip, uint16_t i)
{
    chip->delay = chip->V[X(i)];
}

// Store the current value of the delay timer in register VX
void ldd(Chip8 *chip, uint16_t i)
{
    chip->V[X(i)] = chip->delay;
}

// Wait for a keypress and store the result in register VX
void wait(Chip8 *chip, uint16_t i)
{    
    chip->key_reg = X(i);
}

// Set the sound timer to the value of register VX
void snd(Chip8 *chip, uint16_t i)
{
    chip->sound = chip->V[X(i)];
}

// Add the value stored in register VX to register I
void addi(Chip8 *chip, uint16_t i)
{
    chip->I += chip->V[X(i)];
}

// Set I to the memory address of the sprite data corresponding to the hexadecimal digit stored in register VX
void dgt(Chip8 *chip, uint16_t i)
{   
    chip->I = chip->V[X(i)] * 5;
}

// Store the binary-coded decimal equivalent of the value stored in register VX at addresses I, I+1, and I+2
void bcd(Chip8 *chip, uint16_t i)
{
    // The simplest way to break a decimal number into digits - read the last digit, 
    // divide it out, repeat until done
    uint8_t number = chip->V[X(i)];
    chip->ram[chip->I + 2] = number % 10;

    number /= 10;
    chip->ram[chip->I + 1] = number % 10;

    number /= 10;
    chip->ram[chip->I + 0] = number % 10;
}

// Store the values of registers V0 to VX inclusive in memory starting at address I
// I is set to I + X + 1 after operation
void load(Chip8 *chip, uint16_t i)
{
    memcpy(chip->ram + chip->I, chip->V, X(i) + 1);
    chip->I += X(i) + 1;
}

// Fill registers V0 to VX inclusive with the values stored in memory starting at address I
// I is set to I + X + 1 after operation
void fill(Chip8 *chip, uint16_t i)
{
    memcpy(chip->V, chip->ram + chip->I, X(i) + 1);
    chip->I += X(i) + 1;
}
