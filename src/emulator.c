#include "emulator.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Clock frequency range, in Hz
#define CLOCK_FREQ_MIN 1.0
#define CLOCK_FREQ_DEFAULT 600.0
#define CLOCK_FREQ_MAX 1000.0

// Timer frequency, in Hz
#define TIMER_FREQ 60.0

// Display refresh frequency, in Hz
#define REFRESH_FREQ 60.0

static bool WindowInit(Emulator *emulator);
static bool TimingInit(Emulator *emulator);

bool Emulator_Init(Emulator *emulator)
{
    memset(emulator, 0, sizeof(*emulator));
    
    if (!Chip8_Init(&emulator->chip)) {
        return false;
    }
    if (!TimingInit(emulator)) {
        return false;
    }
    if (!Buzzer_Init(&emulator->buzzer)) {
        return false;
    }
    if (!WindowInit(emulator)) {
        return false;
    }
    if (!Screen_Init(&emulator->screen, emulator)) {
        return false;
    }
    
    return true;
}

bool WindowInit(Emulator *emulator)
{
    emulator->window = SDL_CreateWindow("CHIP-8",
        SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH, SCREEN_HEIGHT,
        SDL_WINDOW_SHOWN
    );
    
    if (emulator->window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        return false;
    }
    
    return true;
}

static Uint64 CalculatePeriod(double freq)
{
    const double ns = 1000000000.0;
    return (Uint64)((1.0 / freq) * ns);
}

static Uint64 TicksToNs(Uint64 t1, Uint64 t2)
{
    const Uint64 ns = 1000000000;
    return ((t2 - t1) * ns) / SDL_GetPerformanceFrequency();
}

bool TimingInit(Emulator *emulator)
{
    emulator->clock_freq = CLOCK_FREQ_DEFAULT;
    
    emulator->clock_period   = CalculatePeriod(CLOCK_FREQ_DEFAULT);
    emulator->timer_period   = CalculatePeriod(TIMER_FREQ);
    emulator->refresh_period = CalculatePeriod(REFRESH_FREQ);
    
    return true;
}

void Emulator_Free(Emulator *emulator)
{
    if (emulator == NULL) {
        return;
    }
    
    free(emulator->rom);
    
    Screen_Free(&emulator->screen);
    Buzzer_Free(&emulator->buzzer);
    Chip8_Free(&emulator->chip);
    
    if (emulator->window) {
        SDL_DestroyWindow(emulator->window);
    }
}

double Emulator_ClockFreq(Emulator *emulator)
{
    return emulator->clock_freq;   
}

double Emulator_TimerFreq(Emulator *emulator)
{
    return TIMER_FREQ;
}

double Emulator_RefreshFreq(Emulator *emulator)
{
    return REFRESH_FREQ;
}

static void LoadProgram(Emulator *emulator, uint8_t *program, size_t size)
{
    // Load the program into a blank chip
    Chip8_Reset(&emulator->chip);
    Chip8_LoadProgram(&emulator->chip, program, size);
    
    // Unpause the emulator so the program runs straight away
    emulator->paused = false;
}

static void LoadDefaultRom(Emulator *emulator)
{
    // This program plays by default when the emulator is started
    static uint8_t demo[] = {
        0x6E, 0x0C, 0x60, 0x88, 0x61, 0x88, 0x62, 0xF8, 0x63, 0x88, 0x64, 0x88, 0xA2, 0x70, 0xF4, 0x55,
        0x60, 0x00, 0x61, 0x00, 0x62, 0xF8, 0x63, 0x00, 0x64, 0x00, 0xF4, 0x55, 0x22, 0x2E, 0x6A, 0x0A,
        0xFA, 0x15, 0xFA, 0x07, 0x3A, 0x00, 0x12, 0x22, 0x22, 0x2E, 0x7E, 0x01, 0x12, 0x1C, 0x60, 0x0C,
        0xF0, 0x29, 0x60, 0x10, 0xD0, 0xE5, 0xA2, 0x70, 0x60, 0x18, 0xD0, 0xE5, 0xA2, 0x75, 0x60, 0x20,
        0xD0, 0xE5, 0x60, 0x08, 0xF0, 0x29, 0x60, 0x28, 0xD0, 0xE5, 0x00, 0xEE,
    };
    
    LoadProgram(emulator, demo, sizeof(demo) / sizeof(demo[0]));
}

uint8_t *ReadRom(const char *path, size_t *size)
{
    // Open the file for reading
    FILE *file = fopen(path, "rb");
    if (file == NULL) {
        fprintf(stderr, "Could not open ROM at '%s'\n", path);
        return NULL;
    }
    
    // Calculate the number of bytes in the file
    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);
    
    // Allocate a buffer for the file contents
    uint8_t *buffer = (uint8_t *)malloc(file_size);
    if (buffer == NULL) {
        fprintf(stderr, "Could not allocate storage for ROM\n");
        return NULL;
    }
    
    // Try to read the entire file into the buffer, else panic
    size_t bytes_read = fread(buffer, sizeof(uint8_t), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read ROM at '%s'\n", path);
        return NULL;
    }
    
    // Done reading the file
    fclose(file);
    
    // Let the caller know how large the file was and return the buffer
    *size = file_size;
    return buffer;
}

static bool LoadRom(Emulator *emulator, const char *path)
{
    // Because CHIP-8 ROMs are not really a file format (they are plain
    // binary files) we have no way of knowing if the provided file
    // even represents a program. To save the user from accidentally
    // "crashing" the emulator in case they try to upload a file of wrong
    // type (such as .png), we expect the ROM to have a certain extension.
    // If something goes wrong, an indicating bell sound should get played.
    char* extension = strchr(path, '.');
    if (!extension || strcmp(extension, ".ch8") != 0) {
        fprintf(stderr, "ROM files should have '.ch8' extension\a\n");
        return false;
    }

    // Try to read the ROM from file, abort if unsuccessful
    size_t size;
    uint8_t *program = ReadRom(path, &size);
    if (program == NULL) {
        return false;
    }
    
    // Destroy previous ROM and set the new one
    free(emulator->rom);
    emulator->rom = program;
    
    // Load the new program
    LoadProgram(emulator, emulator->rom, size);
    return true;
}

bool Emulator_Preload(Emulator *emulator, const char *path)
{
    // If user did not specify path to ROM, load the default one
    if (path == NULL) {
        LoadDefaultRom(emulator);
        return true;
    }
    
    // Otherwise try to load it from file
    return LoadRom(emulator, path);
}

static void Stop(Emulator *emulator)
{
    emulator->running = false;
}

static void Pause(Emulator *emulator)
{
    emulator->paused = !emulator->paused;
}

static void DisableAudio(Emulator *emulator)
{
    Buzzer_Disable(&emulator->buzzer);
}

static void Restart(Emulator *emulator)
{
    // Restart program execution in the chip
    Chip8_Restart(&emulator->chip);
    
    // Unpause the emulator so the program runs straight away
    emulator->paused = false;
}

static void SetFrequency(Emulator *emulator, double value)
{
    // Clamp frequency to the supported range
    if (value < CLOCK_FREQ_MIN) {
        value = CLOCK_FREQ_MIN;
    } else if (value > CLOCK_FREQ_MAX) {
        value = CLOCK_FREQ_MAX;
    }
    
    // Update the clock frequency and period
    emulator->clock_freq = value;
    emulator->clock_period = CalculatePeriod(emulator->clock_freq);
}

static void AddFrequency(Emulator *emulator, double value)
{
    SetFrequency(emulator, emulator->clock_freq + value);
}

static void ResetFrequency(Emulator *emulator)
{
    SetFrequency(emulator, CLOCK_FREQ_DEFAULT);
}

static void MoveCursor(Emulator *emulator, int direction)
{
    // User is only allowed to move the cursor when the program is paused
    if (emulator->paused) {
        Screen_SetMemoryCursor(&emulator->screen, emulator->screen.mem_cursor + direction);
    }
}

static void ProcessWindowEvent(Emulator *emulator, SDL_Event *ev)
{
    switch (ev->window.type) {
        case SDL_WINDOWEVENT_CLOSE: Stop(emulator); break;
    }
}

static void ProcessKeydownEvent(Emulator *emulator, SDL_Event *ev)
{
    switch (ev->key.keysym.sym) {
        case SDLK_ESCAPE: Stop(emulator); break;
        case SDLK_p: Pause(emulator); break;
        case SDLK_0: Restart(emulator); break;
        case SDLK_UP: MoveCursor(emulator, -1); break;
        case SDLK_DOWN: MoveCursor(emulator, 1); break;
        case SDLK_LEFTBRACKET: AddFrequency(emulator, -10.0); break;
        case SDLK_RIGHTBRACKET: AddFrequency(emulator, 10.0); break;
        case SDLK_EQUALS: ResetFrequency(emulator); break;
        case SDLK_BACKSPACE: LoadDefaultRom(emulator); break;
        case SDLK_l: DisableAudio(emulator); break;
    }
}

static void ProcessDropFileEvent(Emulator *emulator, SDL_Event *ev)
{
    // Attempt to load the file specified in the event
    char *file = ev->drop.file;
    if (!LoadRom(emulator, file)) {
        fprintf(stderr, "Could not load ROM from '%s'\a\n", file);
    }
    
    // Discard the file information
    SDL_free(file);
}

static void ProcessEvents(Emulator *emulator)
{
    // Discharge the event queue. We only care about some event types.
    SDL_Event ev;
    while (SDL_PollEvent(&ev) != 0) {
        switch (ev.type) {
            case SDL_QUIT: Stop(emulator); break;
            case SDL_WINDOWEVENT: ProcessWindowEvent(emulator, &ev); break;
            case SDL_KEYDOWN: ProcessKeydownEvent(emulator, &ev); break;
            case SDL_DROPFILE: ProcessDropFileEvent(emulator, &ev); break;
        }
    }
}

static void UpdateKeys(Emulator *emulator)
{
    // Mapping of the CHIP-8 hexpad to modern keyboards
    static const SDL_Keycode mappings[NUM_KEYS] = {
        [0x0] = SDL_SCANCODE_X,
        [0x1] = SDL_SCANCODE_1,
        [0x2] = SDL_SCANCODE_2,
        [0x3] = SDL_SCANCODE_3,
        [0x4] = SDL_SCANCODE_Q,
        [0x5] = SDL_SCANCODE_W,
        [0x6] = SDL_SCANCODE_E,
        [0x7] = SDL_SCANCODE_A,
        [0x8] = SDL_SCANCODE_S,
        [0x9] = SDL_SCANCODE_D,
        [0xA] = SDL_SCANCODE_Z,
        [0xB] = SDL_SCANCODE_C,
        [0xC] = SDL_SCANCODE_4,
        [0xD] = SDL_SCANCODE_R,
        [0xE] = SDL_SCANCODE_F,
        [0xF] = SDL_SCANCODE_V
    };
    
    // Go through each hexpad key and notify the chip of its current state
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    for (uint8_t key = 0; key < NUM_KEYS; key++) {
        Chip8_UpdateKey(&emulator->chip, key, keys[mappings[key]]);
    }
}

static void UpdateTimers(Emulator *emulator)
{
    // Notify the chip that timers must be updated
    Chip8_UpdateTimers(&emulator->chip);
    
    // Buzzing sound must play whenever the sound register is updated
    Buzzer_Play(&emulator->buzzer, emulator->chip.sound);
}

static void UpdateCycle(Emulator *emulator)
{
    // Run a single cycle of execution in the chip
    Chip8_Step(&emulator->chip);
    
    // Update the memory cursor on the screen to highlight the PC
    Screen_SetMemoryCursor(&emulator->screen, emulator->chip.pc);
}

void Emulator_Run(Emulator *emulator)
{
    // Create timestamps for timing
    Uint64 t1 = SDL_GetPerformanceCounter();
    Uint64 t2 = SDL_GetPerformanceCounter();
    
    // Create timing accumulators, in ns. These will keep track of time to
    // determine when to update components of the emulation.
    //
    // For the clock and timers these have a special meaning - they represent
    // the amount of time since the beginning of emulation that they have not
    // used up yet. Each iteration of the main loop may cause multiple updates
    // to both the clock and the timers to "discharge" the accumulators and
    // catch up with the world.
    Uint64 clock_acc = 0;
    Uint64 timer_acc = 0;
    Uint64 refresh_acc = 0;
    
    // Run the main emulation loop
    emulator->running = true;
    while (emulator->running) {
        // Timing
        t2 = SDL_GetPerformanceCounter();
        Uint64 delta_time = TicksToNs(t1, t2);
        t1 = t2;
        
        // Input
        ProcessEvents(emulator);
        
        // Processing
        if (!emulator->paused) {
            UpdateKeys(emulator);
            
            // Discharge the timing accumulator
            timer_acc += delta_time;
            while (timer_acc >= emulator->timer_period) {
                UpdateTimers(emulator);
                timer_acc -= emulator->timer_period;
            }
            
            // Discharge the clock accumulator
            clock_acc += delta_time;
            while (clock_acc >= emulator->clock_period) {
                UpdateCycle(emulator);
                clock_acc -= emulator->clock_period;
            }
        }

        // Render
        // There is no need to discharge the refresh because the emulation
        // state is not changed between render calls.
        refresh_acc += delta_time;
        if (refresh_acc >= emulator->refresh_period) {
            Screen_Refresh(&emulator->screen);
            refresh_acc = 0;
        }
    }
}
