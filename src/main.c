#include <SDL.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "emulator.h"

int main(int argc, char **argv)
{
    // Set the exit status of the program, assuming success by default
    int status = EXIT_SUCCESS;
    
    // Initialize the SDL library subsystems we're using
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
        status = EXIT_FAILURE;
        goto clean_up;
    }

    // We're using the standard random number generator based on the program start-up time
    srand(time(NULL));

    // Create an initialize the emulator
    Emulator emulator;
    if (!Emulator_Init(&emulator)) {
        status = EXIT_FAILURE;
        goto clean_up;
    }

    // Pre-load a ROM if specified
    char *path = (argc > 1) ? argv[1] : NULL;
    if (!Emulator_Preload(&emulator, path)) {
        status = EXIT_FAILURE;
        goto clean_up;
    }

    // Run the emulation
    Emulator_Run(&emulator);

clean_up:
    // Free all resources used by the program
    Emulator_Free(&emulator);
    SDL_Quit();

    return status;
}