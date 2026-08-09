#pragma once

#include <QAudioFormat>
#include <QAudioSink>
#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QMutex>
#include <QObject>

namespace ftremote {

class AudioBuffer final : public QIODevice
{
    Q_OBJECT
public:
    explicit AudioBuffer(QObject *parent = nullptr);
    void append(const QByteArray &data);
    QByteArray takeAll();
    qint64 bytesAvailable() const override;

signals:
    void dataArrived();

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    mutable QMutex m_mutex;
    QByteArray m_data;
};

class AudioEngine final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString inputDevice READ inputDevice NOTIFY devicesChanged)
    Q_PROPERTY(QString outputDevice READ outputDevice NOTIFY devicesChanged)

public:
    explicit AudioEngine(QObject *parent = nullptr);
    ~AudioEngine() override;

    bool running() const { return m_running; }
    QString inputDevice() const { return m_inputDevice.description(); }
    QString outputDevice() const { return m_outputDevice.description(); }

    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void feedRxPcm(const QByteArray &pcm);
    Q_INVOKABLE QStringList inputDevices() const;
    Q_INVOKABLE QStringList outputDevices() const;

signals:
    void runningChanged();
    void devicesChanged();
    void txPcmReady(const QByteArray &pcm);
    void audioError(const QString &message);

private slots:
    void onTxReady();

private:
    QAudioFormat format() const;
    QAudioDevice m_inputDevice;
    QAudioDevice m_outputDevice;
    QAudioSink *m_sink = nullptr;
    QAudioSource *m_source = nullptr;
    AudioBuffer m_rxBuffer;
    AudioBuffer m_txBuffer;
    bool m_running = false;
};

} // namespace ftremote
