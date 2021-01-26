#ifndef BUZZER_H
#define BUZZER_H

#include <SDL.h>

#include <stdint.h>
#include <stdbool.h>

typedef struct Buzzer {
    // Handle to the audio device that emits buzzing sound. Set to 0 if unavailable,
    // in which case no sound is played.
    SDL_AudioDeviceID device;
    
    // Specification of the audio device.
    SDL_AudioSpec spec;
    
    // Buffer where the audio data (samples) is to be stored.
    uint8_t *buffer;
    
    // The number of bytes (not samples) in the audio buffer.
    size_t num_bytes;
    
    // Controls whether the audio is paused.
    bool disabled;
} Buzzer;

bool Buzzer_Init(Buzzer *buzzer);
void Buzzer_Free(Buzzer *buzzer);

// Flips the `disabled` status of the buzzer, so that it either gets disabled or re-enabled.
void Buzzer_Disable(Buzzer *buzzer);

// Updates the buzzer with the current value of the chip's sound register to play the sound.
void Buzzer_Play(Buzzer *buzzer, uint8_t sound);

#endif