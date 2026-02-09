#ifndef VOICEALERTMANAGER_H
#define VOICEALERTMANAGER_H

#include <QObject>
#include <QMap>
#include <QString>
#include <QTimer>
#include <QElapsedTimer>
#include <QProcess>

/**
 * VoiceAlertManager - Modern voice-based system health alert manager.
 *
 * Announces system health status changes using text-to-speech when a
 * subsystem enters a critical or degraded state. Uses platform TTS
 * engines (espeak-ng / espeak / say) for cross-platform speech synthesis.
 *
 * Alert format: "System Status <Critical|Degraded|Warning|Offline> <health>%"
 *
 * Features:
 *   - Per-component cooldown to prevent alert spam
 *   - Configurable alert thresholds and cooldown interval
 *   - Priority queuing (critical > degraded > warning)
 *   - Mute/unmute toggle
 *   - Rate-limited speech to avoid overlapping announcements
 *   - Robust TTS engine detection via QStandardPaths + common paths
 *   - Watchdog timer to recover from hung TTS processes
 */
class VoiceAlertManager : public QObject
{
    Q_OBJECT

public:
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

signals:
    /** Emitted when a voice alert is triggered */
    void alertTriggered(const QString& componentId, const QString& status, qreal healthPercent);

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

    QString resolveHealthStatus(const QString& color) const;
    int     statusPriority(const QString& status) const;
    bool    shouldAlert(const QString& status) const;
    void    speak(const QString& text);
    void    resetSpeakingState();
    QString findTtsEngine() const;

    // Alert queue and processing
    QList<AlertEntry> m_alertQueue;
    QTimer* m_queueTimer;
    bool m_isSpeaking;

    // TTS engine
    QProcess* m_ttsProcess;
    QString m_ttsEngine;

    // Watchdog timer to recover from hung TTS processes
    QTimer* m_watchdogTimer;
    static const int WATCHDOG_TIMEOUT_MS = 10000; // 10 seconds max for any speech

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
