#ifndef VOICEALERTMANAGER_H
#define VOICEALERTMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QProcess>

/**
 * VoiceAlertManager - Robust voice-based system health alert manager.
 *
 * Announces system health status changes using text-to-speech when a
 * subsystem enters a critical or degraded state.
 *
 * Uses a multi-strategy approach to maximize audio reliability:
 *   1. Pipeline: espeak-ng --stdout | aplay  (bypasses PulseAudio issues)
 *   2. WAV file: espeak-ng -w file.wav + aplay file.wav
 *   3. Direct:   espeak-ng "text"  (simplest, may fail with audio config)
 *
 * Alert format: "<ComponentName>, System Status <Level>, <health> percent"
 *
 * Features:
 *   - Multi-strategy TTS with automatic fallback
 *   - Per-component cooldown to prevent alert spam
 *   - Configurable alert thresholds and cooldown interval
 *   - Priority queuing (critical > degraded > warning)
 *   - Mute/unmute toggle with test voice button
 *   - Watchdog timer to recover from hung TTS processes
 *   - Comprehensive startup diagnostics
 */
class VoiceAlertManager : public QObject
{
    Q_OBJECT

public:
    /** TTS playback strategy - ordered by reliability on Linux */
    enum SpeakStrategy {
        ShellPipeline,    // espeak --stdout | aplay (most reliable)
        WavFilePlayback,  // espeak -w file.wav && aplay file.wav
        DirectTTS,        // espeak "text" (simplest but may fail)
        NoStrategy        // No TTS available
    };

    explicit VoiceAlertManager(QObject* parent = nullptr);
    ~VoiceAlertManager();

    /**
     * Process a health update and trigger a voice alert if needed.
     * @param componentId   The component identifier
     * @param componentName Human-readable component name (e.g. "Antenna")
     * @param color         Health status color hex string
     * @param healthPercent Health level 0-100
     */
    void processHealthUpdate(const QString& componentId,
                             const QString& componentName,
                             const QString& color,
                             qreal healthPercent);

    /** Set the minimum seconds between alerts for the same component */
    void setCooldownSeconds(int seconds);
    int cooldownSeconds() const;

    /** Mute / unmute voice alerts */
    void setMuted(bool muted);
    bool isMuted() const;

    /** Enable/disable specific alert levels */
    void setAlertOnCritical(bool enabled);
    void setAlertOnDegraded(bool enabled);
    void setAlertOnWarning(bool enabled);
    void setAlertOnOffline(bool enabled);

    /** Speak a test message to verify audio output works */
    void testVoice();

    /** Check if a working TTS strategy is available */
    bool isTtsAvailable() const;

    /** Get human-readable diagnostic information */
    QString diagnosticInfo() const;

    /** Get the name of the currently selected strategy */
    QString strategyName() const;

signals:
    /** Emitted when a voice alert is triggered */
    void alertTriggered(const QString& componentId, const QString& status, qreal healthPercent);
    /** Emitted when TTS availability status changes */
    void ttsStatusChanged(bool available, const QString& info);

private slots:
    void processQueue();
    void onSpeechFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void onSpeechError(QProcess::ProcessError error);
    void onSpeechWatchdogTimeout();

private:
    struct AlertEntry {
        QString componentId;
        QString componentName;
        QString status;
        qreal   healthPercent;
        int     priority;       // Lower = higher priority
    };

    // Health status resolution
    QString resolveHealthStatus(const QString& color) const;
    int     statusPriority(const QString& status) const;
    bool    shouldAlert(const QString& status) const;

    // Speech output
    void speak(const QString& text);
    void speakWithShellPipeline(const QString& text);
    void speakWithWavFile(const QString& text);
    void speakDirect(const QString& text);
    void resetSpeakingState();
    void cleanupWavFile();

    // Text sanitization for shell commands
    QString shellEscape(const QString& text) const;

    // TTS detection
    QString findTtsEngine() const;
    QString findAudioPlayer() const;
    SpeakStrategy detectBestStrategy() const;
    void logAudioDiagnostics() const;

    // Alert queue and processing
    QList<AlertEntry> m_alertQueue;
    QTimer* m_queueTimer;
    bool m_isSpeaking;

    // TTS engine and audio playback
    QProcess* m_ttsProcess;
    QString m_ttsEngine;
    QString m_audioPlayer;
    SpeakStrategy m_strategy;
    QString m_currentWavFile;

    // Watchdog timer to recover from hung TTS processes
    QTimer* m_watchdogTimer;
    static const int WATCHDOG_TIMEOUT_MS = 15000; // 15 seconds max

    // Per-component cooldown tracking
    QMap<QString, QElapsedTimer*> m_lastAlertTime;
    QMap<QString, QString> m_lastAlertStatus;
    int m_cooldownMs;

    // Configuration
    bool m_muted;
    bool m_alertCritical;
    bool m_alertDegraded;
    bool m_alertWarning;
    bool m_alertOffline;
};

#endif // VOICEALERTMANAGER_H
