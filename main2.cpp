#include <iostream>
#include <cmath>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <SDL2/SDL.h>

const int SAMPLE_RATE = 44100;
const int FREQUENCY = 440;
const int AMPLITUDE = 32760;
const int BUFFER_SIZE = SAMPLE_RATE / 10; // Larger buffer size for smoother playback
const int NUM_BUFFERS = 4; // Number of buffers to queue

enum WaveType { SINE, SQUARE };

void generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency) {
    for (int i = 0; i < length; ++i) {
        float time = static_cast<float>(i) / SAMPLE_RATE;
        if (waveType == SINE) {
            buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
        } else if (waveType == SQUARE) {
            buffer[i] = (i % (SAMPLE_RATE / frequency) < (SAMPLE_RATE / frequency / 2)) ? AMPLITUDE : -AMPLITUDE;
        }
    }
}

void play_wave(ALuint* buffers, ALuint source, WaveType waveType, int frequency) {
    int16_t samples[BUFFER_SIZE];
    generate_wave(samples, waveType, BUFFER_SIZE, frequency);

    // Fill buffers with generated samples
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffers[i]);
    }

    alSourcePlay(source);
}

void update_buffers(ALuint* buffers, ALuint source, WaveType waveType, int frequency) {
    ALint processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, waveType, BUFFER_SIZE, frequency);
        alBufferData(buffer, AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);

        alSourceQueueBuffers(source, 1, &buffer);
        processed--;
    }

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
    }
}

int main(int argc, char* argv[]) {
    // Initialize OpenAL
    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open audio device." << std::endl;
        return 1;
    }

    ALCcontext* context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create OpenAL context." << std::endl;
        alcCloseDevice(device);
        return 1;
    }
    alcMakeContextCurrent(context);

    ALuint buffers[NUM_BUFFERS], source;
    alGenBuffers(NUM_BUFFERS, buffers);
    alGenSources(1, &source);

    // Initialize SDL for keyboard input
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Tone Generator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640, 480, SDL_WINDOW_SHOWN);
    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    bool quit = false;
    SDL_Event e;
    WaveType currentWave = SINE;
    bool playing = false;

    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
            } else if (e.type == SDL_KEYDOWN) {
                switch (e.key.keysym.sym) {
                    case SDLK_s:
                        currentWave = SINE;
                        if (!playing) {
                            play_wave(buffers, source, currentWave, FREQUENCY);
                            playing = true;
                        }
                        break;
                    case SDLK_q:
                        currentWave = SQUARE;
                        if (!playing) {
                            play_wave(buffers, source, currentWave, FREQUENCY);
                            playing = true;
                        }
                        break;
                    case SDLK_SPACE:
                        if (playing) {
                            alSourceStop(source);
                            playing = false;
                        } else {
                            play_wave(buffers, source, currentWave, FREQUENCY);
                            playing = true;
                        }
                        break;
                }
            }
        }

        if (playing) {
            update_buffers(buffers, source, currentWave, FREQUENCY);
        }

        SDL_Delay(1); // Prevent high CPU usage
    }

    // Cleanup
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(NUM_BUFFERS, buffers);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
