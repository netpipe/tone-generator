#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QTimer>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <AL/al.h>
#include <AL/alc.h>
#include <random>
#include <QLabel>
//QT_CHARTS_USE_NAMESPACE

enum WaveType { SINE, SQUARE, WHITE_NOISE, PINK_NOISE, BINAURAL_BEATS };

class ToneGeneratorWidget : public QWidget {
    Q_OBJECT

public:
    ToneGeneratorWidget(QWidget* parent = nullptr);
    ~ToneGeneratorWidget();

private slots:
    void onPlayButtonClicked();
    void onStopButtonClicked();
    void onAudioTimerTimeout();
    void onChartTimerTimeout();
    void onFrequencyChanged();
    void onBeatFrequencyChanged();
    void onWaveTypeChanged(int index);
    void onPresetFrequencyChanged(int index);

private:
    void generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency, int& phase, int frequency2);
    void play_wave(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2);
    void update_buffers(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2);
    void stop_wave(ALuint* buffers, ALuint source);
    void update_chart();

    QPushButton* playButton;
    QPushButton* stopButton;
    QComboBox* waveTypeComboBox;
    QLineEdit* frequencyInput;
    QLineEdit* beatFrequencyInput;
    QComboBox* presetFrequenciesComboBox;
    QTimer* audioTimer;
    QTimer* chartTimer;
    QChartView* chartView;
    QLineSeries* series;

    WaveType currentWave;
    bool playing;
    int frequency;
    int beatFrequency;
    int frequency2;
    int phase;

    ALuint buffers[4];
    ALuint source;
    ALCdevice* device;
    ALCcontext* context;

    static const int SAMPLE_RATE = 44100;
    static const int AMPLITUDE = 32760;
    static const int BUFFER_SIZE = SAMPLE_RATE / 2; // Larger buffer size for smoother playback
};

ToneGeneratorWidget::ToneGeneratorWidget(QWidget* parent)
    : QWidget(parent), playing(false), frequency(440), beatFrequency(10), frequency2(450), phase(0) {
    playButton = new QPushButton("Play", this);
    stopButton = new QPushButton("Stop", this);
    waveTypeComboBox = new QComboBox(this);
    frequencyInput = new QLineEdit(this);
    beatFrequencyInput = new QLineEdit(this);
    presetFrequenciesComboBox = new QComboBox(this);
    audioTimer = new QTimer(this);
    chartTimer = new QTimer(this);
    chartView = new QChartView(this);
    series = new QLineSeries();

    waveTypeComboBox->addItem("Sine", SINE);
    waveTypeComboBox->addItem("Square", SQUARE);
    waveTypeComboBox->addItem("White Noise", WHITE_NOISE);
    waveTypeComboBox->addItem("Pink Noise", PINK_NOISE);
    waveTypeComboBox->addItem("Binaural Beats", BINAURAL_BEATS);

    QStringList presetFrequencies = {"440 Hz", "1000 Hz", "5000 Hz", "10000 Hz"};
    for (const auto& freq : presetFrequencies) {
        presetFrequenciesComboBox->addItem(freq);
    }

    auto layout = new QVBoxLayout;
    layout->addWidget(playButton);
    layout->addWidget(stopButton);
    layout->addWidget(new QLabel("Wave Type:"));
    layout->addWidget(waveTypeComboBox);
    layout->addWidget(new QLabel("Frequency (Hz):"));
    layout->addWidget(frequencyInput);
    layout->addWidget(new QLabel("Beat Frequency (Hz):"));
    layout->addWidget(beatFrequencyInput);
    layout->addWidget(new QLabel("Preset Frequencies:"));
    layout->addWidget(presetFrequenciesComboBox);
    layout->addWidget(chartView);

    setLayout(layout);

    chartView->chart()->addSeries(series);
    chartView->chart()->createDefaultAxes();
    chartView->setRenderHint(QPainter::Antialiasing);

    connect(playButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onPlayButtonClicked);
    connect(stopButton, &QPushButton::clicked, this, &ToneGeneratorWidget::onStopButtonClicked);
    connect(audioTimer, &QTimer::timeout, this, &ToneGeneratorWidget::onAudioTimerTimeout);
    connect(chartTimer, &QTimer::timeout, this, &ToneGeneratorWidget::onChartTimerTimeout);
    connect(frequencyInput, &QLineEdit::editingFinished, this, &ToneGeneratorWidget::onFrequencyChanged);
    connect(beatFrequencyInput, &QLineEdit::editingFinished, this, &ToneGeneratorWidget::onBeatFrequencyChanged);
    connect(waveTypeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToneGeneratorWidget::onWaveTypeChanged);
    connect(presetFrequenciesComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ToneGeneratorWidget::onPresetFrequencyChanged);

    device = alcOpenDevice(nullptr);
    context = alcCreateContext(device, nullptr);
    alcMakeContextCurrent(context);

    alGenBuffers(4, buffers);
    alGenSources(1, &source);
}

ToneGeneratorWidget::~ToneGeneratorWidget() {
    stop_wave(buffers, source);

    alDeleteSources(1, &source);
    alDeleteBuffers(4, buffers);

    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

void ToneGeneratorWidget::onPlayButtonClicked() {
    frequency = frequencyInput->text().toInt();
    beatFrequency = beatFrequencyInput->text().toInt();
    frequency2 = frequency + beatFrequency;
    currentWave = static_cast<WaveType>(waveTypeComboBox->currentData().toInt());

    if (!playing) {
        phase = 0;
        play_wave(buffers, source, currentWave, frequency, phase, frequency2);
        playing = true;
        audioTimer->start(10);
        chartTimer->start(100); // Update chart every 100 ms
    }
}

void ToneGeneratorWidget::onStopButtonClicked() {
    if (playing) {
        stop_wave(buffers, source);
        playing = false;
        audioTimer->stop();
        chartTimer->stop();
        phase = 0;
    }
}

void ToneGeneratorWidget::onAudioTimerTimeout() {
    if (playing) {
        update_buffers(buffers, source, currentWave, frequency, phase, frequency2);
    }
}

void ToneGeneratorWidget::onChartTimerTimeout() {
    if (playing) {
        update_chart();
    }
}

void ToneGeneratorWidget::onFrequencyChanged() {
    if (playing) {
        onStopButtonClicked();
        onPlayButtonClicked();
    }
}

void ToneGeneratorWidget::onBeatFrequencyChanged() {
    if (playing) {
        onStopButtonClicked();
        onPlayButtonClicked();
    }
}

void ToneGeneratorWidget::onWaveTypeChanged(int index) {
    if (playing) {
        onStopButtonClicked();
        onPlayButtonClicked();
    }
}

void ToneGeneratorWidget::onPresetFrequencyChanged(int index) {
    QString freqText = presetFrequenciesComboBox->currentText();
    frequency = freqText.split(' ')[0].toInt();
    frequencyInput->setText(QString::number(frequency));
    onFrequencyChanged();
}

void ToneGeneratorWidget::generate_wave(int16_t* buffer, WaveType waveType, int length, int frequency, int& phase, int frequency2) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(-AMPLITUDE, AMPLITUDE);

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
                buffer[i] = dis(gen) / 2 + buffer[(i - 1 + length) % length] / 2;
                break;
            case BINAURAL_BEATS:
                buffer[i] = static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency * time));
                buffer[i] += static_cast<int16_t>(AMPLITUDE * std::sin(2.0f * M_PI * frequency2 * time));
                buffer[i] /= 2;
                break;
        }
    }
    phase += length;
}

void ToneGeneratorWidget::play_wave(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2) {
    int16_t samples[BUFFER_SIZE];
    generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase, frequency2);

    for (int i = 0; i < 4; ++i) {
        alBufferData(buffers[i], AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffers[i]);
    }

    alSourcePlay(source);
}

void ToneGeneratorWidget::update_buffers(ALuint* buffers, ALuint source, WaveType waveType, int frequency, int& phase, int frequency2) {
    int processed;
    alGetSourcei(source, AL_BUFFERS_PROCESSED, &processed);

    while (processed > 0) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);

        int16_t samples[BUFFER_SIZE];
        generate_wave(samples, waveType, BUFFER_SIZE, frequency, phase, frequency2);
        alBufferData(buffer, AL_FORMAT_MONO16, samples, sizeof(samples), SAMPLE_RATE);
        alSourceQueueBuffers(source, 1, &buffer);

        --processed;
    }

    ALint state;
    alGetSourcei(source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) {
        alSourcePlay(source);
    }
}

void ToneGeneratorWidget::stop_wave(ALuint* buffers, ALuint source) {
    alSourceStop(source);
    alSourcei(source, AL_BUFFER, 0);

    ALint queued;
    alGetSourcei(source, AL_BUFFERS_QUEUED, &queued);

    while (queued--) {
        ALuint buffer;
        alSourceUnqueueBuffers(source, 1, &buffer);
    }
}

void ToneGeneratorWidget::update_chart() {
    QVector<QPointF> points;
    float time = 0.0f;
    float increment = 1.0f / SAMPLE_RATE;

    for (int i = 0; i < BUFFER_SIZE; ++i) {
        points.append(QPointF(time, std::sin(2.0f * M_PI * frequency * time)));
        time += increment;
    }

    series->replace(points);
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ToneGeneratorWidget widget;
    widget.show();

    return app.exec();
}

#include "main.moc"
