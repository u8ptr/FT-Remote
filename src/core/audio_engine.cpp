#include "audio_engine.h"

#include <QMutexLocker>

#include <cstring>

namespace ftremote {

AudioBuffer::AudioBuffer(QObject *parent)
    : QIODevice(parent)
{
    open(QIODevice::ReadWrite);
}

void AudioBuffer::append(const QByteArray &data)
{
    if (data.isEmpty())
        return;
    {
        QMutexLocker locker(&m_mutex);
        constexpr qsizetype maxBytes = 1920 * 10;
        m_data.append(data);
        if (m_data.size() > maxBytes)
            m_data.remove(0, m_data.size() - maxBytes);
    }
    emit readyRead();
    emit dataArrived();
}

QByteArray AudioBuffer::takeAll()
{
    QMutexLocker locker(&m_mutex);
    const QByteArray result = m_data;
    m_data.clear();
    return result;
}

qint64 AudioBuffer::bytesAvailable() const
{
    QMutexLocker locker(&m_mutex);
    return m_data.size() + QIODevice::bytesAvailable();
}

qint64 AudioBuffer::readData(char *data, qint64 maxlen)
{
    QMutexLocker locker(&m_mutex);
    const qint64 count = qMin(maxlen, qint64(m_data.size()));
    if (count > 0) {
        memcpy(data, m_data.constData(), size_t(count));
        m_data.remove(0, int(count));
    }
    return count;
}

qint64 AudioBuffer::writeData(const char *data, qint64 len)
{
    if (len <= 0)
        return 0;
    {
        QMutexLocker locker(&m_mutex);
        m_data.append(data, int(len));
    }
    emit readyRead();
    emit dataArrived();
    return len;
}

AudioEngine::AudioEngine(QObject *parent)
    : QObject(parent), m_inputDevice(QMediaDevices::defaultAudioInput()),
      m_outputDevice(QMediaDevices::defaultAudioOutput()), m_rxBuffer(this), m_txBuffer(this)
{
    connect(&m_txBuffer, &AudioBuffer::dataArrived, this, &AudioEngine::onTxReady);
}

AudioEngine::~AudioEngine()
{
    stop();
}

QAudioFormat AudioEngine::format() const
{
    QAudioFormat audioFormat;
    audioFormat.setSampleRate(48000);
    audioFormat.setChannelCount(1);
    audioFormat.setSampleFormat(QAudioFormat::Int16);
    return audioFormat;
}

void AudioEngine::start()
{
    if (m_running)
        return;
    const QAudioFormat audioFormat = format();
    if (!m_outputDevice.isFormatSupported(audioFormat) || !m_inputDevice.isFormatSupported(audioFormat)) {
        emit audioError(QStringLiteral("默认音频设备不支持 48 kHz mono S16，未启动音频"));
        return;
    }
    m_sink = new QAudioSink(m_outputDevice, audioFormat, this);
    m_source = new QAudioSource(m_inputDevice, audioFormat, this);
    m_sink->start(&m_rxBuffer);
    m_source->start(&m_txBuffer);
    if (m_sink->error() != QtAudio::Error::NoError || m_source->error() != QtAudio::Error::NoError) {
        emit audioError(QStringLiteral("音频设备启动失败"));
        stop();
        return;
    }
    m_running = true;
    emit runningChanged();
}

void AudioEngine::stop()
{
    if (m_sink)
        m_sink->stop();
    if (m_source)
        m_source->stop();
    delete m_sink;
    delete m_source;
    m_sink = nullptr;
    m_source = nullptr;
    m_rxBuffer.takeAll();
    m_txBuffer.takeAll();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
}

void AudioEngine::feedRxPcm(const QByteArray &pcm)
{
    if (m_running)
        m_rxBuffer.append(pcm);
}

QStringList AudioEngine::inputDevices() const
{
    QStringList result;
    for (const auto &device : QMediaDevices::audioInputs())
        result.append(device.description());
    return result;
}

QStringList AudioEngine::outputDevices() const
{
    QStringList result;
    for (const auto &device : QMediaDevices::audioOutputs())
        result.append(device.description());
    return result;
}

void AudioEngine::onTxReady()
{
    const QByteArray pcm = m_txBuffer.takeAll();
    if (!pcm.isEmpty())
        emit txPcmReady(pcm);
}

} // namespace ftremote
