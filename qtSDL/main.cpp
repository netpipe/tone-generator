#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

const int SAMPLE_RATE = 44100;
const int FREQUENCY = 440;
const int AMPLITUDE = 32760;
const int BUFFER_SIZE = 4096; // Larger buffer size for smoother playback

enum WaveType { SINE, SQUARE };

int phase = 0;
WaveType currentWave = SINE;
bool playing = false;

void generate_wave(int16_t* buffer, int length, int frequency, WaveType waveType, int& phase) {
    for (int i = 0; i < length; ++i) {
        float time = static_cast<float>(phase + i) / SAMPLE_RATE;
        if (waveType == SINE) {
            buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
        } else if (waveType == SQUARE) {
            float period = static_cast<float>(SAMPLE_RATE) / frequency;
            buffer[i] = ((phase + i) % static_cast<int>(period) < (period / 2)) ? AMPLITUDE : -AMPLITUDE;
        }
    }
    phase += length;
}

void audio_callback(void* userdata, Uint8* stream, int len) {
    int16_t* buffer = reinterpret_cast<int16_t*>(stream);
    int length = len / 2; // len is in bytes, we need the number of samples
    generate_wave(buffer, length, FREQUENCY, currentWave, phase);
}

void start_audio() {
    if (!playing) {
        SDL_PauseAudio(0);
        playing = true;
    }
}

void stop_audio() {
    if (playing) {
        SDL_PauseAudio(1);
        playing = false;
    }
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Initialize SDL
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // Initialize SDL_mixer
    if (Mix_OpenAudio(SAMPLE_RATE, AUDIO_S16SYS, 1, BUFFER_SIZE) < 0) {
        std::cerr << "Failed to open audio: " << Mix_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    Mix_AllocateChannels(1);
    Mix_ChannelFinished(nullptr);

    // Register the audio callback
    SDL_AudioSpec desiredSpec;
    SDL_zero(desiredSpec);
    desiredSpec.freq = SAMPLE_RATE;
    desiredSpec.format = AUDIO_S16SYS;
    desiredSpec.channels = 1;
    desiredSpec.samples = BUFFER_SIZE;
    desiredSpec.callback = audio_callback;

    if (SDL_OpenAudio(&desiredSpec, nullptr) < 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        Mix_CloseAudio();
        SDL_Quit();
        return 1;
    }

    // Create the main window
    QWidget window;
    window.setWindowTitle("Tone Generator");

    QVBoxLayout *layout = new QVBoxLayout();

    QPushButton *sineButton = new QPushButton("Play Sine Wave");
    QPushButton *squareButton = new QPushButton("Play Square Wave");
    QPushButton *stopButton = new QPushButton("Stop");

    layout->addWidget(sineButton);
    layout->addWidget(squareButton);
    layout->addWidget(stopButton);

    QObject::connect(sineButton, &QPushButton::clicked, []() {
        currentWave = SINE;
        start_audio();
    });

    QObject::connect(squareButton, &QPushButton::clicked, []() {
        currentWave = SQUARE;
        start_audio();
    });

    QObject::connect(stopButton, &QPushButton::clicked, []() {
        stop_audio();
    });

    window.setLayout(layout);
    window.show();

    int result = app.exec();

    // Cleanup
    stop_audio();
    Mix_CloseAudio();
    SDL_Quit();

    return result;
}

