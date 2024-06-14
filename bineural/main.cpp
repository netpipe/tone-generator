#include <iostream>
#include <cmath>
#include <vector>
#include <random>
#include <AL/al.h>
#include <AL/alc.h>
#include <QApplication>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QKeyEvent>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

// this is a WIP so be careful before using

//QT_CHARTS_USE_NAMESPACE

const int SAMPLE_RATE = 44100;
const int AMPLITUDE = 32760;
const int BUFFER_SIZE = 512; // Smaller buffer size for smoother playback
const int NUM_BUFFERS = 4; // Number of buffers to queue

enum WaveType { SINE, SQUARE, WHITE_NOISE, PINK_NOISE, BINAURAL_BEATS };

void generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency, int& phase, int frequency2 = 0) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(-AMPLITUDE, AMPLITUDE);

    for (int i = 0; i < length; ++i) {
        float time = static_cast<float>(phase + i) / SAMPLE_RATE;
        switch (waveType) {
            case SINE:
                buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
                break;
            case SQUARE:
                buffer[i] = ((phase + i) % (SAMPLE_RATE / frequency) < (SAMPLE_RATE / frequency / 2)) ? AMPLITUDE : -AMPLITUDE;
                break;
            case WHITE_NOISE:
                buffer[i] = dis(gen);
                break;
            case PINK_NOISE:
                // Simple approximation of pink noise (white noise filtered)
                buffer[i] = (buffer[i - 1] + dis(gen)) / 2;
                break;
            case BINAURAL_BEATS:
                if (i % 2 == 0) {
                    buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
                } else {
                    buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency2 * time));
                }
                break;
        }
    }
    phase += length;
}

void play_wave(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2 = 0) {
    int16_t samples[BUFFER_SIZE];
    generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase, frequency2);

    // Fill buffers with generated samples
    for (int i = 0; i < NUM_BUFFERS; ++i) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffers[i]);
    }

    alSourcePlay(source);
}

void update_buffers(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2 = 0) {
    ALint processed = 0;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase, frequency2);
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
    ToneGeneratorWidget(QWidget *parent = nullptr) : QWidget(parent), phase(0), currentWave(SINE), playing(false), frequency(440), beatFrequency(10), frequency2(450) {
        QVBoxLayout *layout = new QVBoxLayout();
        QLabel *label = new QLabel("Base Frequency (Hz):");
        frequencyInput = new QLineEdit();
        frequencyInput->setText(QString::number(440));
        QLabel *beatLabel = new QLabel("Beat Frequency (Hz):");
        beatFrequencyInput = new QLineEdit();
        beatFrequencyInput->setText(QString::number(10));
        presetFrequencies = new QComboBox();
        presetFrequencies->addItems({"440", "528", "10000"});
        QPushButton *sineButton = new QPushButton("Play Sine Wave");
        QPushButton *squareButton = new QPushButton("Play Square Wave");
        QPushButton *whiteNoiseButton = new QPushButton("Play White Noise");
        QPushButton *pinkNoiseButton = new QPushButton("Play Pink Noise");
        QPushButton *binauralButton = new QPushButton("Play Binaural Beats");
        QPushButton *stopButton = new QPushButton("Stop");

        layout->addWidget(label);
        layout->addWidget(frequencyInput);
        layout->addWidget(beatLabel);
        layout->addWidget(beatFrequencyInput);
        layout->addWidget(presetFrequencies);
        layout->addWidget(sineButton);
        layout->addWidget(squareButton);
        layout->addWidget(whiteNoiseButton);
        layout->addWidget(pinkNoiseButton);
        layout->addWidget(binauralButton);
        layout->addWidget(stopButton);
        setLayout(layout);

        // Chart setup
        series = new QLineSeries();
        chart = new QChart();
        chart->addSeries(series);
        chart->createDefaultAxes();
        chart->setTitle("Waveform Visualization");

        chartView = new QChartView(chart);
        layout->addWidget(chartView);

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
        connect(whiteNoiseButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onWhiteNoiseButtonClicked);
        connect(pinkNoiseButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onPinkNoiseButtonClicked);
        connect(binauralButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onBinauralButtonClicked);
        connect(stopButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onStopButtonClicked);
        connect(presetFrequencies, &QComboBox::currentTextChanged, this, &ToneGeneratorWidget::onPresetFrequencyChanged);

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
            currentWave = SINE;
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

    void onBeatFrequencyChanged(const QString &text) {
        beatFrequency = text.toInt();
        frequency2 = frequency + beatFrequency;
    }

    void onPresetFrequencyChanged(const QString &text) {
        frequencyInput->setText(text);
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

    void onWhiteNoiseButtonClicked() {
        currentWave = WHITE_NOISE;
        if (!playing) {
            phase = 0; // Reset phase when starting playback
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    }

    void onPinkNoiseButtonClicked() {
        currentWave = PINK_NOISE;
        if (!playing) {
            phase = 0; // Reset phase when starting playback
            play_wave(buffers, source, currentWave, frequency, phase);
            playing = true;
        }
    }

    void onBinauralButtonClicked() {
        frequency = frequencyInput->text().toInt(); // Update frequency when button is clicked
        frequency2 = frequency + beatFrequency; // Update frequency2 for binaural beats
        currentWave = BINAURAL_BEATS;
        if (!playing) {
            phase = 0; // Reset phase when starting playback
            play_wave(buffers, source, currentWave, frequency, phase, frequency2);
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
            update_buffers(buffers, source, currentWave, frequency, phase, frequency2);
            update_chart(); // Update the chart with the current waveform
        }
    }

    void update_chart() {
        QVector<QPointF> points;
        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, currentWave, BUFFER_SIZE, frequency, phase, frequency2);
        for (int i = 0; i < BUFFER_SIZE; ++i) {
            points.append(QPointF(i, samples[i]));
        }
        series->replace(points);
    }

private:
    QLineEdit *frequencyInput;
    QLineEdit *beatFrequencyInput;
    QComboBox *presetFrequencies;
    QTimer *timer;
    int phase;
    WaveType currentWave;
    bool playing;
    int frequency;
    int beatFrequency;
    int frequency2;

    ALCdevice *device;
    ALCcontext *context;
    ALuint buffers[NUM_BUFFERS], source;

    QChart *chart;
    QLineSeries *series;
    QChartView *chartView;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ToneGeneratorWidget window;
    window.setWindowTitle("Advanced Tone Generator");
    window.resize(600, 400);
    window.show();

    return app.exec();
}

#include "main.moc"

