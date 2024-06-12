#include <iostream>
#include <cmath>
#include <SDL2/SDL.h>

const int SAMPLE_RATE = 44100;
const int FREQUENCY = 440;
const int AMPLITUDE = 28000;
const int BUFFER_SIZE = 2048;

enum WaveType { SINE, SQUARE };

WaveType currentWave = SINE;
bool playing = false;

void generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency, int& phase) {
    for (int i = 0; i < length; ++i) {
        float time = static_cast<float>(phase + i) / SAMPLE_RATE;
        if (waveType == SINE) {
            buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
        } else if (waveType == SQUARE) {
            buffer[i] = (i % (SAMPLE_RATE / frequency) < (SAMPLE_RATE / frequency / 2)) ? AMPLITUDE : -AMPLITUDE;
        }
    }
    phase += length;
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    static int phase = 0;
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int length = len / sizeof(int16_t);
    generate_wave(buffer, currentWave, length, FREQUENCY, phase);
}

int main(int argc, char* argv[]) {
    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_AudioSpec desiredSpec, obtainedSpec;
    SDL_memset(&desiredSpec, 0, sizeof(desiredSpec));
    desiredSpec.freq = SAMPLE_RATE;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = BUFFER_SIZE;
    desiredSpec.callback = audio_callback;

    if (SDL_OpenAudio(&desiredSpec, &obtainedSpec) < 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Tone Generator",
                                          SDL_WINDOWPOS_UNDEFINED,
                                          SDL_WINDOWPOS_UNDEFINED,
                                          640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) {
        std::cerr << "Failed to create SDL renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_s:
                        currentWave = SINE;
                        if (!playing) {
                            SDL_PauseAudio(0); // Start audio playback
                            playing = true;
                        }
                        break;
                    case SDLK_q:
                        currentWave = SQUARE;
                        if (!playing) {
                            SDL_PauseAudio(0); // Start audio playback
                            playing = true;
                        }
                        break;
                    case SDLK_SPACE:
                        if (playing) {
                            SDL_PauseAudio(1); // Stop audio playback
                            playing = false;
                        } else {
                            SDL_PauseAudio(0); // Start audio playback
                            playing = true;
                        }
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        SDL_RenderClear(renderer);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

        SDL_Rect sineButton = { 100, 100, 200, 50 };
        SDL_Rect squareButton = { 100, 200, 200, 50 };
        SDL_Rect toggleButton = { 100, 300, 200, 50 };

        SDL_RenderFillRect(renderer, &sineButton);
        SDL_RenderFillRect(renderer, &squareButton);
        SDL_RenderFillRect(renderer, &toggleButton);

        SDL_RenderPresent(renderer);
    }

    SDL_CloseAudio();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

