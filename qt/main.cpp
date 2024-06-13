#include <iostream>
#include <cmath>
#include <vector>
#include <AL/al.h>
#include <AL/alc.h>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QWidget>
#include <QLabel>
#include <QTimer>

const int SAMPLE_RATE = 44100;
const int AMPLITUDE = 32760;
const int BUFFER_SIZE = 4096; // Larger buffer size for smoother playback
const int NUM_BUFFERS = 4; // Number of buffers to queue

enum WaveType { SINE, SQUARE };

void generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency, int& phase) {
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

void play_wave(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase) {
    int16_t samples[BUFFER_SIZE];
    generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase);

    // Fill buffers with generated samples
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffers[i]);
    }

    alSourcePlay(source);
}

void update_buffers(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase) {
    ALint processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase);
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
    QApplication app(argc, argv);

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

    // Create the main window
    QWidget window;
    window.setWindowTitle("Tone Generator");

    QVBoxLayout *layout = new QVBoxLayout();
    QLabel *label = new QLabel("Frequency (Hz):");
    QLineEdit *frequencyInput = new QLineEdit();
    frequencyInput->setText(QString::number(440));
    QPushButton *sineButton = new QPushButton("Play Sine Wave");
    QPushButton *squareButton = new QPushButton("Play Square Wave");
    QPushButton *stopButton = new QPushButton("Stop");

    layout->addWidget(label);
    layout->addWidget(frequencyInput);
    layout->addWidget(sineButton);
    layout->addWidget(squareButton);
    layout->addWidget(stopButton);
    window.setLayout(layout);

    int phase = 0;
    WaveType currentWave = SINE;
    bool playing = false;
    int frequency = 440;

    QObject::connect(frequencyInput, &QLineEdit::textChanged, [&](const QString &text) {
        frequency = text.toInt();
    });

    QObject::connect(sineButton, &QPushButton::clicked, [&]() {
        currentWave = SINE;
        if (!playing) {
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    });

    QObject::connect(squareButton, &QPushButton::clicked, [&]() {
        currentWave = SQUARE;
        if (!playing) {
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    });

    QObject::connect(stopButton, &QPushButton::clicked, [&]() {
        if (playing) {
            alSourceStop(source);
            playing = false;
            phase = 0; // Reset phase when stopping playback
        }
    });

    window.show();

    // Start a timer to update the buffers
    QTimer timer;
    QObject::connect(&timer, &QTimer::timeout, [&]() {
        if (playing) {
            update_buffers(buffers, source, currentWave, frequency, phase);
        }
    });
    timer.start(10); // Update every 10 ms

    int result = app.exec();

    // Cleanup
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(NUM_BUFFERS, buffers);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);

    return result;
}

