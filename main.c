
/**
 * WAV File Player
 * 
 * A simple C program that reads and plays WAV audio files using SDL2
 */
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <SDL2/SDL.h>

const int window_width = 800;
const int window_height = 300;
const int button_width = 80;
const int button_height = 40;

typedef struct {
    SDL_Rect rect;
    SDL_Color color;
    SDL_Color hover_color;
    bool hovered;
    const char* label;
} Button;

typedef struct {
    uint8_t* buffer;
    uint32_t length;
    uint32_t position;
    SDL_AudioSpec spec;
    SDL_AudioDeviceID device;
    bool playing;
} AudioState;

// Function prototypes
void initialize_buttons(Button* buttons, SDL_Renderer* renderer);
void handle_buttons(AudioState* audio, Button* buttons, SDL_Point mouse_pos);
void audio_callback(void* userdata, Uint8* stream, int len);
int kbhit(void);
uint16_t read_uint16(FILE *file);
uint32_t read_uint32(FILE *file);
int play_wav_file(const char *file_path);

void initialize_buttons(Button* buttons, SDL_Renderer* renderer) {
    const int spacing = 10;
    int x = (window_width - (3 * button_width + 2 * spacing)) / 2;
    int y = window_height - button_height - 10;

    for(int i=0; i<3; i++) {
        buttons[i].rect = (SDL_Rect){
            x + i*(button_width + spacing), 
            y, 
            button_width, 
            button_height
        };
        buttons[i].color = (SDL_Color){200, 200, 200, 255};
        buttons[i].hover_color = (SDL_Color){150, 150, 150, 255};
        buttons[i].hovered = false;
    }

    buttons[0].label = "Pause";
    buttons[1].label = "Rewind";
    buttons[2].label = "FFwd";
}

void handle_buttons(AudioState* audio, Button* buttons, SDL_Point mouse_pos) {
    for (int i = 0; i < 3; i++) {
        if (SDL_PointInRect(&mouse_pos, &buttons[i].rect)) {
            switch(i) {
                case 0: // Pause/Play
                    audio->playing = !audio->playing;
                    SDL_PauseAudioDevice(audio->device, !audio->playing);
                    buttons[0].label = audio->playing ? "Pause" : "Play";
                    break;
                case 1: // Rewind
                    audio->position = audio->position > 5000 ? 
                        audio->position - 5000 : 0;
                    break;
                case 2: // Fast-forward
                    audio->position = audio->position + 5000 < audio->length ? 
                        audio->position + 5000 : audio->length - 1;
                    break;
            }
            break;
        }
    }
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    AudioState* audio = (AudioState*)userdata;
    uint32_t bytes_remaining = audio->length - audio->position;
    uint32_t bytes_to_copy = (uint32_t)len > bytes_remaining ? 
        bytes_remaining : (uint32_t)len;

    SDL_memcpy(stream, audio->buffer + audio->position, bytes_to_copy);
    audio->position += bytes_to_copy;

    if (bytes_to_copy < (uint32_t)len) {
        SDL_memset(stream + bytes_to_copy, 0, len - bytes_to_copy);
        audio->playing = false;
        SDL_PauseAudioDevice(audio->device, 1);
    }
}

// Function to check if a key is pressed (non-blocking)
int kbhit(void) {
    // Using SDL's event queue to check for keyboard input
    SDL_Event event;
    int key_pressed = 0;
    
    // Process all pending events
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_KEYDOWN) {
            key_pressed = 1;
            break;
        }
    }
    
    return key_pressed;
}

// Read a little-endian 16-bit value
uint16_t read_uint16(FILE *file) {
    uint16_t value;
    uint8_t bytes[2];
    if (fread(bytes, 1, 2, file) != 2) {
        return 0;
    }
    value = (bytes[1] << 8) | bytes[0];
    return value;
}

// Read a little-endian 32-bit value
uint32_t read_uint32(FILE *file) {
    uint32_t value;
    uint8_t bytes[4];
    if (fread(bytes, 1, 4, file) != 4) {
        return 0;
    }
    value = (bytes[3] << 24) | (bytes[2] << 16) | (bytes[1] << 8) | bytes[0];
    return value;
}

// Function to read WAV file and play it
int play_wav_file(const char *file_path) {
    // Open WAV file
    FILE *wav_file = fopen(file_path, "rb");
    if (wav_file == NULL) {
        printf("'%s' inaccessible\n", file_path);
        return 1;
    }
    
    // Read RIFF header
    char chunk_id[5] = {0};
    uint32_t chunk_size;
    char format[5] = {0};
    
    if (fread(chunk_id, 1, 4, wav_file) != 4) {
        printf("RIFF header inaccessible\n");
        fclose(wav_file);
        return 1;
    }
    
    chunk_size = read_uint32(wav_file);
    
    if (fread(format, 1, 4, wav_file) != 4) {
        printf("Failed to read format\n");
        fclose(wav_file);
        return 1;
    }
    
    // Validate RIFF header
    if (strncmp(chunk_id, "RIFF", 4) != 0 || strncmp(format, "WAVE", 4) != 0) {
        printf("Invalid WAV file format\n");
        fclose(wav_file);
        return 1;
    }
    
    // Read format chunk
    char subchunk1_id[5] = {0};
    uint32_t subchunk1_size;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    
    // Find "fmt " chunk
    while (1) {
        if (fread(subchunk1_id, 1, 4, wav_file) != 4) {
            printf("Failed to find format chunk\n");
            fclose(wav_file);
            return 1;
        }
        
        subchunk1_size = read_uint32(wav_file);
        
        if (strncmp(subchunk1_id, "fmt ", 4) == 0) {
            break;
        }
        
        // Skip this chunk
        fseek(wav_file, subchunk1_size, SEEK_CUR);
    }
    
    // Read format information
    audio_format = read_uint16(wav_file);
    num_channels = read_uint16(wav_file);
    sample_rate = read_uint32(wav_file);
    byte_rate = read_uint32(wav_file);
    block_align = read_uint16(wav_file);
    bits_per_sample = read_uint16(wav_file);
    
    // Skip any extra format bytes
    if (subchunk1_size > 16) {
        fseek(wav_file, subchunk1_size - 16, SEEK_CUR);
    }
    
    // Find "data" chunk
    char subchunk2_id[5] = {0};
    uint32_t subchunk2_size;
    
    while (1) {
        if (fread(subchunk2_id, 1, 4, wav_file) != 4) {
            printf("Failed to find data chunk\n");
            fclose(wav_file);
            return 1;
        }
        
        subchunk2_size = read_uint32(wav_file);
        
        if (strncmp(subchunk2_id, "data", 4) == 0) {
            break;
        }
        
        // Skip this chunk
        fseek(wav_file, subchunk2_size, SEEK_CUR);
    }
    
    // Print WAV file information
    printf("WAV File Information:\n");
    printf("Channels: %u\n", num_channels);
    printf("Sample Rate: %u Hz\n", sample_rate);
    printf("Bit Depth: %u bits\n", bits_per_sample);
    printf("Audio Format: %u (1 = PCM)\n", audio_format);
    printf("Data Size: %u bytes\n", subchunk2_size);
    
    // Validate PCM format
    if (audio_format != 1) {
        printf("Only PCM format is supported\n");
        fclose(wav_file);
        return 1;
    }
    
    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_EVENTS) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        fclose(wav_file);
        return 1;
    }
    
    // Prepare SDL audio specifications
    SDL_AudioSpec wav_spec;
    wav_spec.freq = sample_rate;
    
    // Set the correct SDL audio format based on bits per sample
    switch (bits_per_sample) {
        case 8:
            wav_spec.format = AUDIO_U8;
            break;
        case 16:
            wav_spec.format = AUDIO_S16SYS;
            break;
        case 24:
        case 32:
            wav_spec.format = AUDIO_S32SYS;
            break;
        default:
            printf("Unsupported bit depth: %u\n", bits_per_sample);
            fclose(wav_file);
            SDL_Quit();
            return 1;
    }
    
    wav_spec.channels = num_channels;
    wav_spec.samples = 4096; // Buffer size
    wav_spec.callback = NULL; // Using SDL_QueueAudio instead of callback
    wav_spec.userdata = NULL;
    
    // Open audio device
    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(NULL, 0, &wav_spec, NULL, 0);
    if (audio_device == 0) {
        printf("Failed to open audio device: %s\n", SDL_GetError());
        fclose(wav_file);
        SDL_Quit();
        return 1;
    }
    
    // Read audio data
    uint8_t *audio_data = (uint8_t*)malloc(subchunk2_size);
    if (audio_data == NULL) {
        printf("Failed to allocate memory for audio data\n");
        SDL_CloseAudioDevice(audio_device);
        fclose(wav_file);
        SDL_Quit();
        return 1;
    }
    
    if (fread(audio_data, 1, subchunk2_size, wav_file) != subchunk2_size) {
        printf("Failed to read audio data\n");
        free(audio_data);
        SDL_CloseAudioDevice(audio_device);
        fclose(wav_file);
        SDL_Quit();
        return 1;
    }
    
    // Start playback
    SDL_PauseAudioDevice(audio_device, 0);
    
    // Queue audio data for playback
    if (SDL_QueueAudio(audio_device, audio_data, subchunk2_size) != 0) {
        printf("Failed to queue audio: %s\n", SDL_GetError());
        free(audio_data);
        SDL_CloseAudioDevice(audio_device);
        fclose(wav_file);
        SDL_Quit();
        return 1;
    }
    
    // Calculate duration in milliseconds: (data_size * 1000) / (sample_rate * channels * bytes_per_sample)
    int bytes_per_sample = bits_per_sample / 8;
    uint32_t duration_ms = (subchunk2_size * 1000) / (sample_rate * num_channels * bytes_per_sample);
    
    printf("Playing audio... Duration: %u ms\n", duration_ms);
    printf("Press any key to stop playback.\n");
    
    // Play audio and wait for key press or completion
    uint32_t start_time = SDL_GetTicks();
    int playing = 1;
    
    while (playing) {
        // Check if audio has finished playing
        uint32_t current_time = SDL_GetTicks();
        if (current_time - start_time >= duration_ms) {
            printf("Playback completed.\n");
            break;
        }
        
        // Check if a key was pressed
        if (kbhit()) {
            printf("Playback stopped by user.\n");
            break;
        }
        
        // Small delay to prevent CPU hogging
        SDL_Delay(10);
    }
    
    // Clean up
    free(audio_data);
    SDL_CloseAudioDevice(audio_device);
    fclose(wav_file);
    SDL_Quit();
    
    return 0;
}

int main(int argc, char *argv[]) {
    // Check if file path is provided
    if (argc > 2) {
        printf("Usage: %s <wav_file_path>\n", argv[0]);
        return 1;
    } else if (argc == 1){
        SDL_Window* window = NULL;
        SDL_Renderer* renderer = NULL;
        SDL_Event event;
        Button buttons[3];
        AudioState audio = {0};
        bool quit = false;

        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
        window = SDL_CreateWindow("Wav Reader", 
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            window_width, window_height, SDL_WINDOW_ALLOW_HIGHDPI);
        renderer = SDL_CreateRenderer(window, -1, 
            SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

        initialize_buttons(buttons, renderer);

        while (!quit) {
            while (SDL_PollEvent(&event)) {
                if (event.type == SDL_QUIT) {
                    quit = true;
                }
                else if (event.type == SDL_DROPFILE) {
                    char* file = event.drop.file;
                    
                    // Load WAV file
                    SDL_AudioSpec spec;
                    uint8_t* buffer;
                    uint32_t length;
                    if (SDL_LoadWAV(file, &spec, &buffer, &length)) {
                        audio.spec = spec;
                        audio.buffer = buffer;
                        audio.length = length;
                        audio.position = 0;
                        audio.playing = true;
                        audio.spec.callback = audio_callback;
                        audio.spec.userdata = &audio;
                        audio.device = SDL_OpenAudioDevice(NULL, 0, 
                            &audio.spec, NULL, 0);
                        SDL_PauseAudioDevice(audio.device, 0);
                        buttons[0].label = "Pause";
                    }
                    SDL_free(file);
                }
                else if (event.type == SDL_MOUSEBUTTONDOWN) {
                    SDL_Point mouse_pos = {event.button.x, event.button.y};
                    handle_buttons(&audio, buttons, mouse_pos);
                }
            }

            // Update hover state for buttons
            int mouseX, mouseY;
            SDL_GetMouseState(&mouseX, &mouseY);
            SDL_Point mouse_pos = {mouseX, mouseY};
            for (int i = 0; i < 3; i++) {
                buttons[i].hovered = SDL_PointInRect(&mouse_pos, &buttons[i].rect);
            }

            // Clear screen
            SDL_SetRenderDrawColor(renderer, 51, 51, 51, 255);
            SDL_RenderClear(renderer);

            // Draw buttons
            for (int i = 0; i < 3; i++) {
                SDL_SetRenderDrawColor(renderer, 
                    buttons[i].color.r, buttons[i].color.g, 
                    buttons[i].color.b, buttons[i].color.a);
                SDL_RenderFillRect(renderer, &buttons[i].rect);
                
                // Draw button label (would need TTF for proper text rendering)
                // For simplicity, we're just drawing placeholder rectangles
            }

            SDL_RenderPresent(renderer);
        }

        if (audio.buffer) SDL_FreeWAV(audio.buffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
    } else {
        return play_wav_file(argv[1]);
    }
}
