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
#include <QKeyEvent>

const int SAMPLE_RATE = 44100;
const int AMPLITUDE = 32760;
const int BUFFER_SIZE = 512; // Smaller buffer size for smoother playback
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

void stop_wave(ALuint* buffers, ALuint source) {
    alSourceStop(source);
    ALint queued;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

    while (queued--) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
    }
}

class ToneGeneratorWidget : public QWidget {
    Q_OBJECT

public:
    ToneGeneratorWidget(QWidget *parent = nullptr) : QWidget(parent), phase(0), currentWave(SINE), playing(false), frequency(440) {
        QVBoxLayout *layout = new QVBoxLayout();
        QLabel *label = new QLabel("Frequency (Hz):");
        frequencyInput = new QLineEdit();
        frequencyInput->setText(QString::number(440));
        QPushButton *sineButton = new QPushButton("Play Sine Wave");
        QPushButton *squareButton = new QPushButton("Play Square Wave");
        QPushButton *stopButton = new QPushButton("Stop");

        layout->addWidget(label);
        layout->addWidget(frequencyInput);
        layout->addWidget(sineButton);
        layout->addWidget(squareButton);
        layout->addWidget(stopButton);
        setLayout(layout);

        // Initialize OpenAL
        device = alcOpenDevice(nullptr);
        if (!device) {
            std::cerr << "Failed to open audio device." << std::endl;
            return;
        }

        context = alcCreateContext(device, nullptr);
        if (!context) {
            std::cerr << "Failed to create OpenAL context." << std::endl;
            alcCloseDevice(device);
            return;
        }
        alcMakeContextCurrent(context);

        alGenBuffers(NUM_BUFFERS, buffers);
        alGenSources(1, &source);

        connect(sineButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onSineButtonClicked);
        connect(squareButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onSquareButtonClicked);
        connect(stopButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onStopButtonClicked);

        // Start a timer to update the buffers
        timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &ToneGeneratorWidget::onTimerTimeout);
        timer->start(10); // Update every 10 ms
    }

    ~ToneGeneratorWidget() {
        stop_wave(buffers, source);
        alDeleteSources(1, &source);
        alDeleteBuffers(NUM_BUFFERS, buffers);

        alcMakeContextCurrent(nullptr);
        alcDestroyContext(context);
        alcCloseDevice(device);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (event->key() == Qt::Key_Y) {
            frequency = 10000;
            currentWave = SQUARE;
            if (!playing) {
                phase = 0; // Reset phase when starting playback
                play_wave(buffers, source, currentWave, frequency, phase);
                playing = true;
            }
        }
        QWidget::keyPressEvent(event);
    }

private slots:
    void onFrequencyChanged(const QString &text) {
        frequency = text.toInt();
    }

    void onSineButtonClicked() {
        frequency = frequencyInput->text().toInt(); // Update frequency when button is clicked
        currentWave = SINE;
        if (!playing) {
            phase = 0; // Reset phase when starting playback
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    }

    void onSquareButtonClicked() {
        frequency = frequencyInput->text().toInt(); // Update frequency when button is clicked
        currentWave = SQUARE;
        if (!playing) {
            phase = 0; // Reset phase when starting playback
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    }

    void onStopButtonClicked() {
        if (playing) {
            stop_wave(buffers, source);
            playing = false;
            phase = 0; // Reset phase when stopping playback
        }
    }

    void onTimerTimeout() {
        if (playing) {
            update_buffers(buffers, source, currentWave, frequency, phase);
        }
    }

private:
    QLineEdit *frequencyInput;
    QTimer *timer;
    int phase;
    WaveType currentWave;
    bool playing;
    int frequency;

    ALCdevice *device;
    ALCcontext *context;
    ALuint buffers[NUM_BUFFERS], source;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ToneGeneratorWidget window;
    window.setWindowTitle("Tone Generator");
    window.resize(400, 200);
    window.show();

    return app.exec();
}

#include "main.moc"

