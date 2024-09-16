#include <QApplication>
#include <QMainWindow>
#include <QAudioOutput>
#include <QVBoxLayout>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QTimer>
#include <QByteArray>
#include <QIODevice>
#include <QtMath>

class Generator : public QIODevice {
    Q_OBJECT

public:
    Generator(const QAudioFormat &format, int sampleRate, bool isSquareWave)
        : m_format(format), m_sampleRate(sampleRate), m_isSquareWave(isSquareWave) {
        generateData();
    }

    void start() { open(QIODevice::ReadOnly); }
    void stop() { m_pos = 0; close(); }

protected:
    qint64 readData(char *data, qint64 len) override {
        qint64 total = 0;
        if (!m_buffer.isEmpty()) {
            while (len - total > 0) {
                const qint64 chunk = qMin((m_buffer.size() - m_pos), len - total);
                memcpy(data + total, m_buffer.constData() + m_pos, chunk);
                m_pos = (m_pos + chunk) % m_buffer.size();
                total += chunk;
            }
        }
        return total;
    }

    qint64 writeData(const char *, qint64) override { return 0; }
    qint64 bytesAvailable() const override { return m_buffer.size() + QIODevice::bytesAvailable(); }

private:
    void generateData() {
        const int channelBytes = m_format.sampleSize() / 8;  // Calculate bytes per sample
        const int sampleBytes = m_format.channelCount() * channelBytes;
        const int durationUs = 1000000;  // 1 second
        qint64 length = (m_format.sampleRate() * m_format.channelCount() * (m_format.sampleSize() / 8) * durationUs) / 1000000;
        m_buffer.resize(length);
        unsigned char *ptr = reinterpret_cast<unsigned char *>(m_buffer.data());

        int sampleIndex = 0;
        while (length) {
            qreal x;
            if (m_isSquareWave) {
                x = (sampleIndex % (m_format.sampleRate() / m_sampleRate) < (m_format.sampleRate() / m_sampleRate) / 2) ? 1.0 : -1.0;
            } else {
                x = qSin(2 * M_PI * m_sampleRate * qreal(sampleIndex++ % m_format.sampleRate()) / m_format.sampleRate());
            }

            for (int i = 0; i < m_format.channelCount(); ++i) {
                switch (m_format.sampleType()) {
                    case QAudioFormat::UnSignedInt:
                        *reinterpret_cast<quint8 *>(ptr) = static_cast<quint8>((1.0 + x) / 2 * 255);
                        break;
                    case QAudioFormat::SignedInt:
                        *reinterpret_cast<qint16 *>(ptr) = static_cast<qint16>(x * 32767);
                        break;
                    case QAudioFormat::Float:
                        *reinterpret_cast<float *>(ptr) = x;
                        break;
                    default:
                        break;
                }
                ptr += channelBytes;
                length -= channelBytes;
            }
            sampleIndex++;
        }
    }

    QByteArray m_buffer;
    qint64 m_pos = 0;
    QAudioFormat m_format;
    int m_sampleRate;
    bool m_isSquareWave;
};

class AudioTest : public QMainWindow {
    Q_OBJECT

public:
    AudioTest() {
        initializeWindow();
        initializeAudio();
    }

private slots:
    void startStop() {
        if (m_audioOutput->state() == QAudio::StoppedState) {
            m_generator->start();
            m_audioOutput->start(m_generator.get());
            m_startStopButton->setText("Stop");
        } else {
            m_generator->stop();
            m_audioOutput->stop();
            m_startStopButton->setText("Start");
        }
    }

    void waveformChanged(int index) {
        m_generator.reset(new Generator(m_audioOutput->format(), m_frequencyLineEdit->text().toInt(), index == 1));
    }

    void frequencyChanged() {
        m_generator.reset(new Generator(m_audioOutput->format(), m_frequencyLineEdit->text().toInt(), m_waveformBox->currentIndex() == 1));
    }

private:
    void initializeWindow() {
        QWidget *window = new QWidget;
        QVBoxLayout *layout = new QVBoxLayout;

        m_waveformBox = new QComboBox(this);
        m_waveformBox->addItem("Sine Wave");
        m_waveformBox->addItem("Square Wave");
        connect(m_waveformBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &AudioTest::waveformChanged);
        layout->addWidget(m_waveformBox);

        m_frequencyLineEdit = new QLineEdit(this);
        m_frequencyLineEdit->setText("440");
        connect(m_frequencyLineEdit, &QLineEdit::editingFinished, this, &AudioTest::frequencyChanged);
        layout->addWidget(new QLabel("Frequency (Hz):"));
        layout->addWidget(m_frequencyLineEdit);

        m_startStopButton = new QPushButton("Start", this);
        connect(m_startStopButton, &QPushButton::clicked, this, &AudioTest::startStop);
        layout->addWidget(m_startStopButton);

        window->setLayout(layout);
        setCentralWidget(window);
        window->show();
    }

    void initializeAudio() {
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(1);
        format.setSampleSize(16);  // In Qt 5.12, sample size is set separately from format
        format.setCodec("audio/pcm");
        format.setByteOrder(QAudioFormat::LittleEndian);
        format.setSampleType(QAudioFormat::SignedInt);

        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        if (!info.isFormatSupported(format)) {
            //qWarning() << "Default format not supported, trying to use nearest.";
            format = info.nearestFormat(format);
        }

        m_audioOutput = std::make_unique<QAudioOutput>(format, this);
        m_generator = std::make_unique<Generator>(format, m_frequencyLineEdit->text().toInt(), m_waveformBox->currentIndex() == 1);
    }

    std::unique_ptr<Generator> m_generator;
    std::unique_ptr<QAudioOutput> m_audioOutput;
    QPushButton *m_startStopButton;
    QComboBox *m_waveformBox;
    QLineEdit *m_frequencyLineEdit;
};

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AudioTest audioTest;
    audioTest.show();
    return app.exec();
}

#include "main.moc"
