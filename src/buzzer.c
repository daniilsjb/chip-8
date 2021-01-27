#include "buzzer.h"

#include <stdio.h>

bool Buzzer_Init(Buzzer *buzzer)
{
    memset(buzzer, 0, sizeof(*buzzer));
    
    // The desired audio specification
    SDL_AudioSpec want = {
        .freq = 64 * 60,
        .format = AUDIO_F32LSB,
        .channels = 1,
        .samples = 64
    };
    
    // If no devices could be explicitly determined, then the audio system won't be used
    if (SDL_GetNumAudioDevices(0) <= 0) {
        fprintf(stderr, "Could not find available audio devices, no sound will be played\n");
        return true;
    }
    
    // Open any available device
    buzzer->device = SDL_OpenAudioDevice(NULL, 0, &want, &buzzer->spec, SDL_AUDIO_ALLOW_ANY_CHANGE);
    
    // If nothing opened, then the audio system won't be used
    if (buzzer->device == 0) {
        fprintf(stderr, "Could not open an audio device, no sound will be played\n");
        return true;
    }
    
    // Unpause the device to allow playback
    SDL_PauseAudioDevice(buzzer->device, 0);
    
    // Store 4 bytes of audio data per sample per channel
    buzzer->num_bytes = 4 * buzzer->spec.samples * buzzer->spec.channels;
    
    // Allocate the audio buffer or panic
    if (!(buzzer->buffer = (uint8_t *)malloc(buzzer->num_bytes))) {
        fprintf(stderr, "Could not allocate audio buffer\n");
        return false;
    }
    
    return true;
}

void Buzzer_Free(Buzzer *buzzer)
{
    if (buzzer == NULL) {
        return;
    }
    
    free(buzzer->buffer);
    
    if (buzzer->device) {
        SDL_CloseAudioDevice(buzzer->device);
    }
}

void Buzzer_Disable(Buzzer *buzzer)
{
    buzzer->disabled = !buzzer->disabled;
}

void Buzzer_Play(Buzzer *buzzer, uint8_t sound)
{
    // If the audio system is not in use, do nothing
    if (buzzer->device == 0) {
        return;
    }
    
    // We need to convert audio data (float) into its raw 32-bit representation.
    // To achieve this we are using type punning: data is written into this union
    // as float, then same bits are read as uint32.
    union {
        float number;
        uint32_t bits;
    } data;
    
    // Write sound or no sound depending on the chip's sound register and current settings
    data.number = (sound > 0 && !buzzer->disabled) ? 1.0f : 0.0f;
    
    // Copy the audio bits into 4-byte samples
    uint8_t samples[4] = { 0 };
    memcpy(samples, &data.bits, 4);
    
    // Fill the audio buffer with the samples
    for (size_t i = 0; i < buzzer->num_bytes; i += 4) {
        memcpy(buzzer->buffer + i, samples, 4);
    }
    
    // Send the audio buffer to the audio device for further playback
    if (SDL_QueueAudio(buzzer->device, buzzer->buffer, buzzer->num_bytes) != 0) {
        fprintf(stderr, "Audio Error: %s\n", SDL_GetError());
    }
}
