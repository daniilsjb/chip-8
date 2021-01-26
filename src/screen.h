#ifndef SCREEN_H
#define SCREEN_H

#include <SDL.h>

#include <stdbool.h>

// Window dimensions
#define SCREEN_WIDTH 1200
#define SCREEN_HEIGHT 800

// Forward declaration to avoid circular dependencies
typedef struct Emulator Emulator;

typedef struct Screen {
    // The main renderer used to display graphics. Owned by the screen.
    SDL_Renderer *renderer;
    
    // Bitmap atlas representing the font.
    SDL_Texture *font;
    
    // Texture target for chip's video display. It contains 64x32 logical
    // pixels corresponding to the chip's video buffer, but will be drawn
    // over a larger area on the screen, causing pixels to appear larger.
    SDL_Texture *video;
    
    // Absolute offsets into chip's RAM that specify the segment of memory
    // the user will see displayed on the screen. These will be updated to
    // follow the `mem_cursor` field.
    int mem_begin, mem_end;
    
    // Currently highlighted memory address. This field may be controlled
    // by the chip's program counter or manually by the user if the program
    // is on pause.
    int mem_cursor;
    
    // Reference to the emulator being displayed.
    Emulator *emulator;
} Screen;

bool Screen_Init(Screen *screen, Emulator *emulator);
void Screen_Free(Screen *screen);

// Sets the currently highlighted memory address to the given value and
// updates the displayed memory segment.
void Screen_SetMemoryCursor(Screen *screen, int address);

// Renders the current frame.
void Screen_Refresh(Screen *screen);

#endif