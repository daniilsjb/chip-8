#include "screen.h"

#include <string.h>
#include <stdio.h>

#include "emulator.h"

// Dimensions of a single glyph in the font atlas
#define GLYPH_W 10
#define GLYPH_H 14

// Horizontal and vertical spacing for text
#define TEXT_SPACING_H 2
#define TEXT_SPACING_V 8

// The size of character buffers used for writing formatted strings
#define FMT_SIZE 128

// The number of memory cells that may be displayed at once
#define MEM_BREADTH 16

static bool RendererInit(Screen *screen, SDL_Window *window);
static bool FontInit(Screen *screen);
static bool VideoInit(Screen *screen);
static bool CursorInit(Screen *screen);

bool Screen_Init(Screen *screen, Emulator *emulator)
{
    memset(screen, 0, sizeof(*screen));
    screen->emulator = emulator;
    
    if (!RendererInit(screen, emulator->window)) {
        return false;
    }
    if (!FontInit(screen)) {
        return false;
    }
    if (!VideoInit(screen)) {
        return false;
    }
    if (!CursorInit(screen)) {
        return false;
    }
    
    return true;
}

bool RendererInit(Screen *screen, SDL_Window *window)
{
    screen->renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (screen->renderer == NULL) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

bool FontInit(Screen *screen)
{
    // The font is an ordinary bitmap file
    SDL_Surface *surface = SDL_LoadBMP("res/fonts/c8-font.bmp");
    if (surface == NULL) {
        fprintf(stderr, "Could not load font: %s\n", SDL_GetError());
        return false;
    }
    
    // Set the transparency color in the font, since BMP format lacks the alpha channel
    SDL_SetColorKey(surface, SDL_TRUE, SDL_MapRGB(surface->format, 0xFF, 0xFF, 0xFF));
    
    // Create a texture from the bitmap so we can quickly render it
    screen->font = SDL_CreateTextureFromSurface(screen->renderer, surface);
    if (screen->font == NULL) {
        fprintf(stderr, "Could not use font: %s\n", SDL_GetError());
        return false;
    }
    
    // The font is now stored on the GPU, free the surface
    SDL_FreeSurface(surface);
    return true;
}

bool VideoInit(Screen *screen)
{
    screen->video = SDL_CreateTexture(screen->renderer,
        SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
        VIDEO_WIDTH, VIDEO_HEIGHT
    );
    
    if (screen->video == NULL) {
        fprintf(stderr, "Could not create video texture: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

static bool CursorInit(Screen *screen)
{
    // The distance between `mem_begin` and `mem_end` must always be `MEM_BREADTH`
    screen->mem_begin = 0;
    screen->mem_end = MEM_BREADTH;
    
    // Initially cursor just points to the start of memory
    screen->mem_cursor = 0;
    
    return true;
}

void Screen_Free(Screen *screen)
{
    if (screen == NULL) {
        return;
    }
    
    if (screen->video) {
        SDL_DestroyTexture(screen->video);
    }
    if (screen->font) {
        SDL_DestroyTexture(screen->font);
    }
    if (screen->renderer) {
        SDL_DestroyRenderer(screen->renderer);
    }
}

void Screen_SetMemoryCursor(Screen *screen, int address)
{
    // Clamp the address to valid range
    if (address < 0) {
        address = 0;
    }
    if (address >= RAM_SIZE) {
        address = RAM_SIZE;
    }
    
    // Update the cursor with the new address
    screen->mem_cursor = address;
    
    // If the new address goes beyond the memory segment, move the segment to it
    if (address < screen->mem_begin) {
        screen->mem_begin = address;
        screen->mem_end = address + MEM_BREADTH;
    }
    if (address > screen->mem_end) {
        screen->mem_end = address;
        screen->mem_begin = address - MEM_BREADTH;
    }
}

static void DrawLetter(Screen *screen, char ch, int x, int y)
{
    // Only draw characters that the font supports
    if (ch < '!' || ch > '~') {
        return;
    }
    
    // Calculate the glyph position within the atlas
    int gx = (ch - 33) % 16;
    int gy = (ch - 33) / 16;
    
    // Copy the glyph's sub-rectangle into the desired position on the screen
    SDL_Rect src = (SDL_Rect) { gx * GLYPH_W, gy * GLYPH_H, GLYPH_W, GLYPH_H };
    SDL_Rect dst = (SDL_Rect) { x,            y,            GLYPH_W, GLYPH_H };

    SDL_RenderCopy(screen->renderer, screen->font, &src, &dst);
}

static void DrawString(Screen *screen, const char *string, int x, int y)
{
    // Character offsets for calculating positions relative to the string
    int dx = 0;
    int dy = 0;
    
    // Draw the string char-by-char until we hit the null-terminator
    for (char ch = *string; ch != '\0'; ch = *++string) {
        // On space simply advance the offset forward
        if (ch == ' ') {
            dx += 1;
            continue;
        }
        
        // On newline advance the offset to the beginning of the next line
        if (ch == '\n') {
            dx = 0;
            dy += 1;
            continue;
        }
        
        // Calculate character's absolute position on the screen
        int cx = x + (GLYPH_W + TEXT_SPACING_H) * dx;
        int cy = y + (GLYPH_H + TEXT_SPACING_V) * dy;
        
        // Draw it and advance forward
        DrawLetter(screen, ch, cx, cy);
        dx += 1;
    }
}

static void DisplayVideo(Screen *screen);
static void DisplayInformation(Screen *screen);
static void DisplayFrequencies(Screen *screen);
static void DisplayMemory(Screen *screen);
static void DisplayRegisters(Screen *screen);

void Screen_Refresh(Screen *screen)
{
    // Draw background
    SDL_SetRenderDrawColor(screen->renderer, 8, 24, 32, 255);
    SDL_RenderClear(screen->renderer);
    
    // Draw emulator components
    DisplayVideo(screen);
    DisplayInformation(screen);
    DisplayFrequencies(screen);
    DisplayMemory(screen);
    DisplayRegisters(screen);
    
    // Render everything to the window
    SDL_RenderPresent(screen->renderer);
}

static void DrawBorder(Screen *screen, int x, int y, int w, int h)
{
    // Draw the highlight
    SDL_SetRenderDrawColor(screen->renderer, 0, 0, 0, 255);
    SDL_RenderDrawLine(screen->renderer, x, y, x + w, y    );
    SDL_RenderDrawLine(screen->renderer, x, y, x    , y + h);
    
    // Draw the rest
    SDL_SetRenderDrawColor(screen->renderer, 136, 192, 112, 255);
    SDL_RenderDrawLine(screen->renderer, x + w, y    , x + w, y + h);
    SDL_RenderDrawLine(screen->renderer, x    , y + h, x + w, y + h);
}

void DisplayVideo(Screen *screen)
{
    DrawBorder(screen, 6, 6, 772, 388);
    
    // Render VRAM data to a texture
    SDL_SetRenderTarget(screen->renderer, screen->video);
    
    // Clear the screen
    SDL_SetRenderDrawColor(screen->renderer, 109, 145, 93, 255);
    SDL_RenderClear(screen->renderer);
    
    // Get a reference to the chip for simplicity
    Chip8 *chip = &screen->emulator->chip;
    
    // Draw pixels
    SDL_SetRenderDrawColor(screen->renderer, 8, 24, 32, 255);
    for (int y = 0; y < VIDEO_HEIGHT; y++) {
        for (int x = 0; x < VIDEO_WIDTH; x++) {
            // Left-most pixels are stored starting from most-significant bit.
            // If a bit is set, draw its corresponding pixel.
            if (chip->video[y] & (1ull << (VIDEO_WIDTH - x - 1))) {
                SDL_RenderDrawPoint(screen->renderer, x, y);
            }
        }
    }
    
    // Restore the default render target
    SDL_SetRenderTarget(screen->renderer, NULL);
    
    // Copy the entire texture into the video section of the window
    SDL_Rect dst = (SDL_Rect) { 8, 8, 768, 384 };
    SDL_RenderCopy(screen->renderer, screen->video, NULL, &dst);
}

void DisplayInformation(Screen *screen)
{
    // The information message is constant
    static const char *message =
        "Welcome to the CHIP-8 Emulator!\n"
        "Made by daniilsjb\n"
        "\n"
        "Use the following keys:\n"
        "  - 'P' to pause or unpause\n"
        "  - '0' to restart the current ROM\n"
        "  - '[' to decrease clock frequency\n"
        "  - ']' to increase clock frequency\n"
        "  - '=' to reset clock frequency\n"
        "  - 'L' to disable or enable sound \n"
        "  - Up/Down arrows when paused to move memory cursor\n"
        "  - Backspace to load the default ROM\n"
        "\n"
        "You may load another ROM from a file by dragging and dropping\n"
        "it over this window.\n";
    
    // Simply display the message
    DrawBorder(screen, 6, 400, 772, 352);
    DrawString(screen, message, 12, 414);
}

void DisplayFrequencies(Screen *screen)
{
    DrawBorder(screen, 6, 760, 772, 34);
    
    // Text buffer for formatting
    char buffer[FMT_SIZE];
    
    // Retrieve and display the frequencies from the emulator
    snprintf(buffer, FMT_SIZE, "Clock @ %g Hz", Emulator_ClockFreq(screen->emulator));
    DrawString(screen, buffer, 12, 770);
    
    snprintf(buffer, FMT_SIZE, "Timers @ %g Hz", Emulator_TimerFreq(screen->emulator));
    DrawString(screen, buffer, 300, 770);
    
    snprintf(buffer, FMT_SIZE, "Refresh @ %g Hz", Emulator_RefreshFreq(screen->emulator));
    DrawString(screen, buffer, 590, 770);
}

void DisplayMemory(Screen *screen)
{
    DrawBorder(screen, 784, 6, 410, 388);
    
    // Calculate the highlight rectangle; the cursor must be relative to the RAM display
    int cursor = screen->mem_cursor - screen->mem_begin;
    SDL_Rect highlight = (SDL_Rect) { 784, 12 + (GLYPH_H + TEXT_SPACING_V) * cursor, 410, GLYPH_H + 8 };
    
    // Draw the highlight
    if (screen->emulator->paused) {
        SDL_SetRenderDrawColor(screen->renderer, 130, 61, 59, 255);
    } else {
        SDL_SetRenderDrawColor(screen->renderer, 224, 248, 208, 255);
    }
    SDL_RenderFillRect(screen->renderer, &highlight);
    
    // Get a reference to the chip for simplicity
    Chip8 *chip = &screen->emulator->chip;
    
    // Text buffer for formatting
    char buffer[FMT_SIZE];
    
    // Display the contents of RAM in the segment
    for (int i = screen->mem_begin; i <= screen->mem_end; i++) {
        snprintf(buffer, FMT_SIZE, "$%04X    $%02X", i, chip->ram[i]);
        DrawString(screen, buffer, 792, 16 + (GLYPH_H + TEXT_SPACING_V) * (i - screen->mem_begin));
    }
}

void DisplayRegisters(Screen *screen)
{
    DrawBorder(screen, 784, 400, 410, 394);
    
    // Get a reference to the chip for simplicity
    Chip8 *chip = &screen->emulator->chip;
    
    // Text buffer for formatting
    char buffer[FMT_SIZE];
    
    // Draw general-purpose registers
    for (int i = 0; i < NUM_REGISTERS; i++) {
        snprintf(buffer, FMT_SIZE, "V%X: $%02X", i, chip->V[i]);
        DrawString(screen, buffer, 798, 424 + (GLYPH_H + TEXT_SPACING_V) * i);
    }
    
    // Draw special-purpose registers
    snprintf(buffer, FMT_SIZE, " I: $%03X", chip->I);
    DrawString(screen, buffer, 910, 424 + (GLYPH_H + TEXT_SPACING_V) * 0);
    
    snprintf(buffer, FMT_SIZE, "DL: $%X", chip->delay);
    DrawString(screen, buffer, 910, 424 + (GLYPH_H + TEXT_SPACING_V) * 2);
    
    snprintf(buffer, FMT_SIZE, "SD: $%X", chip->sound);
    DrawString(screen, buffer, 910, 424 + (GLYPH_H + TEXT_SPACING_V) * 3);
    
    snprintf(buffer, FMT_SIZE, "PC: $%02X", chip->pc);
    DrawString(screen, buffer, 910, 424 + (GLYPH_H + TEXT_SPACING_V) * 5);
    
    snprintf(buffer, FMT_SIZE, "SP: $%X", chip->sp);
    DrawString(screen, buffer, 910, 424 + (GLYPH_H + TEXT_SPACING_V) * 6);
    
    // Draw the stack
    for (int i = 0; i < NUM_STACK; i++) {
        snprintf(buffer, FMT_SIZE, "ST[%X]: $%03X", i, chip->stack[i]);
        DrawString(screen, buffer, 1042, 424 + (GLYPH_H + TEXT_SPACING_V) * i);
    }
}
