#include <iostream>
#include <cmath>
#include <vector>
#include <QtWidgets/QApplication>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>
#include <AL/al.h>
#include <AL/alc.h>

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

class ToneGenerator : public QWidget {
    Q_OBJECT

public:
    ToneGenerator(QWidget *parent = nullptr);
    ~ToneGenerator();

private slots:
    void startSineWave();
    void startSquareWave();
    void togglePlayback();

private:
    void play_wave(WaveType waveType, int frequency);
    void update_buffers();

    WaveType currentWave;
    bool playing;
    ALuint buffers[NUM_BUFFERS];
    ALuint source;
    QTimer *timer;
};

ToneGenerator::ToneGenerator(QWidget *parent) : QWidget(parent), currentWave(SINE), playing(false) {
    // Initialize OpenAL
    ALCdevice* device = alcOpenDevice(nullptr);
    if (!device) {
        std::cerr << "Failed to open audio device." << std::endl;
        exit(1);
    }

    ALCcontext* context = alcCreateContext(device, nullptr);
    if (!context) {
        std::cerr << "Failed to create OpenAL context." << std::endl;
        alcCloseDevice(device);
        exit(1);
    }
    alcMakeContextCurrent(context);

    alGenBuffers(NUM_BUFFERS, buffers);
    alGenSources(1, &source);

    // Initialize UI
    QVBoxLayout *layout = new QVBoxLayout(this);
    QPushButton *sineButton = new QPushButton("Play Sine Wave", this);
    QPushButton *squareButton = new QPushButton("Play Square Wave", this);
    QPushButton *toggleButton = new QPushButton("Toggle Playback", this);

    connect(sineButton, &QPushButton::clicked, this, &ToneGenerator::startSineWave);
    connect(squareButton, &QPushButton::clicked, this, &ToneGenerator::startSquareWave);
    connect(toggleButton, &QPushButton::clicked, this, &ToneGenerator::togglePlayback);

    layout->addWidget(sineButton);
    layout->addWidget(squareButton);
    layout->addWidget(toggleButton);

    setLayout(layout);

    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &ToneGenerator::update_buffers);
}

ToneGenerator::~ToneGenerator() {
    timer->stop();
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(NUM_BUFFERS, buffers);

    ALCcontext *context = alcGetCurrentContext();
    ALCdevice *device = alcGetContextsDevice(context);
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void ToneGenerator::startSineWave() {
    currentWave = SINE;
    if (!playing) {
        play_wave(currentWave, FREQUENCY);
        playing = true;
    }
}

void ToneGenerator::startSquareWave() {
    currentWave = SQUARE;
    if (!playing) {
        play_wave(currentWave, FREQUENCY);
        playing = true;
    }
}

void ToneGenerator::togglePlayback() {
    if (playing) {
        alSourceStop(source);
        playing = false;
        timer->stop();
    } else {
        play_wave(currentWave, FREQUENCY);
        playing = true;
        timer->start(10); // Update buffers every 10 ms
    }
}

void ToneGenerator::play_wave(WaveType waveType, int frequency) {
    int16_t samples[BUFFER_SIZE];
    generate_wave(samples, waveType, BUFFER_SIZE, frequency);

    // Fill buffers with generated samples
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffers[i]);
    }

    alSourcePlay(source);
    timer->start(10); // Update buffers every 10 ms
}

void ToneGenerator::update_buffers() {
    ALint processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, currentWave, BUFFER_SIZE, FREQUENCY);
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

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    ToneGenerator generator;
    generator.setWindowTitle("Tone Generator");
    generator.resize(300, 200);
    generator.show();

    return app.exec();
}

#include "main.moc"
