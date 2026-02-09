#include "voicealertmanager.h"
#include <QDebug>
#include <QFileInfo>

VoiceAlertManager::VoiceAlertManager(QObject* parent)
    : QObject(parent)
    , m_queueTimer(new QTimer(this))
    , m_isSpeaking(false)
    , m_ttsProcess(nullptr)
    , m_cooldownMs(15000)   // 15 second cooldown per component
    , m_muted(false)
    , m_alertCritical(true)
    , m_alertDegraded(true)
    , m_alertWarning(false)  // Off by default - too noisy
    , m_alertOffline(true)
{
    // Locate available TTS engine on the system
    m_ttsEngine = findTtsEngine();
    if (m_ttsEngine.isEmpty()) {
        qWarning() << "[VoiceAlert] No TTS engine found. Voice alerts will be logged only.";
        qWarning() << "[VoiceAlert] Install espeak-ng or espeak for voice alerts:";
        qWarning() << "[VoiceAlert]   sudo apt-get install espeak-ng";
    } else {
        qDebug() << "[VoiceAlert] Using TTS engine:" << m_ttsEngine;
    }

    // Queue timer processes pending alerts at a steady rate
    m_queueTimer->setInterval(500);
    connect(m_queueTimer, &QTimer::timeout, this, &VoiceAlertManager::processQueue);
    m_queueTimer->start();
}

VoiceAlertManager::~VoiceAlertManager()
{
    if (m_ttsProcess) {
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(1000);
        delete m_ttsProcess;
    }

    // Clean up elapsed timers
    qDeleteAll(m_lastAlertTime);
    m_lastAlertTime.clear();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void VoiceAlertManager::processHealthUpdate(const QString& componentId,
                                            const QString& componentName,
                                            const QString& color,
                                            qreal healthPercent)
{
    QString status = resolveHealthStatus(color);

    // Only alert on configured status levels
    if (!shouldAlert(status)) {
        return;
    }

    // Check per-component cooldown
    if (m_lastAlertTime.contains(componentId)) {
        QElapsedTimer* timer = m_lastAlertTime[componentId];
        QString lastStatus = m_lastAlertStatus.value(componentId);

        // Allow immediate re-alert if status worsened (higher priority)
        bool statusWorsened = statusPriority(status) < statusPriority(lastStatus);

        if (!statusWorsened && timer->elapsed() < m_cooldownMs) {
            return; // Still in cooldown
        }
    }

    // Record the alert time
    if (!m_lastAlertTime.contains(componentId)) {
        m_lastAlertTime[componentId] = new QElapsedTimer();
    }
    m_lastAlertTime[componentId]->start();
    m_lastAlertStatus[componentId] = status;

    // Build alert entry
    AlertEntry entry;
    entry.componentId   = componentId;
    entry.componentName = componentName;
    entry.status        = status;
    entry.healthPercent = healthPercent;
    entry.priority      = statusPriority(status);

    // Insert into queue sorted by priority (lower number = higher priority)
    int insertIdx = 0;
    for (int i = 0; i < m_alertQueue.size(); ++i) {
        if (m_alertQueue[i].priority > entry.priority) {
            break;
        }
        insertIdx = i + 1;
    }

    // Avoid duplicate queued alerts for the same component
    for (int i = 0; i < m_alertQueue.size(); ++i) {
        if (m_alertQueue[i].componentId == componentId) {
            m_alertQueue.removeAt(i);
            if (insertIdx > i) insertIdx--;
            break;
        }
    }

    m_alertQueue.insert(insertIdx, entry);
}

void VoiceAlertManager::setCooldownSeconds(int seconds)
{
    m_cooldownMs = seconds * 1000;
}

int VoiceAlertManager::cooldownSeconds() const
{
    return m_cooldownMs / 1000;
}

void VoiceAlertManager::setMuted(bool muted)
{
    m_muted = muted;
    qDebug() << "[VoiceAlert]" << (muted ? "Muted" : "Unmuted");
}

bool VoiceAlertManager::isMuted() const
{
    return m_muted;
}

void VoiceAlertManager::setAlertOnCritical(bool enabled) { m_alertCritical = enabled; }
void VoiceAlertManager::setAlertOnDegraded(bool enabled) { m_alertDegraded = enabled; }
void VoiceAlertManager::setAlertOnWarning(bool enabled)  { m_alertWarning = enabled; }
void VoiceAlertManager::setAlertOnOffline(bool enabled)  { m_alertOffline = enabled; }

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void VoiceAlertManager::processQueue()
{
    if (m_isSpeaking || m_alertQueue.isEmpty() || m_muted) {
        return;
    }

    AlertEntry entry = m_alertQueue.takeFirst();

    // Build the spoken text: "System Status Critical 45%"
    int healthRounded = qRound(entry.healthPercent);
    QString spokenText = QString("System Status %1, %2 percent")
                             .arg(entry.status)
                             .arg(healthRounded);

    qDebug() << "[VoiceAlert] ALERT:" << entry.componentName
             << "-" << spokenText;

    speak(spokenText);

    emit alertTriggered(entry.componentId, entry.status, entry.healthPercent);
}

void VoiceAlertManager::onSpeechFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitCode)
    Q_UNUSED(exitStatus)
    m_isSpeaking = false;

    if (m_ttsProcess) {
        m_ttsProcess->deleteLater();
        m_ttsProcess = nullptr;
    }
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

QString VoiceAlertManager::resolveHealthStatus(const QString& color) const
{
    QString c = color.toLower();
    if (c == "#00ff00") return "Operational";
    if (c == "#ffff00") return "Warning";
    if (c == "#ffa500") return "Degraded";
    if (c == "#ff0000") return "Critical";
    if (c == "#808080") return "Offline";
    return "Unknown";
}

int VoiceAlertManager::statusPriority(const QString& status) const
{
    // Lower number = higher urgency
    if (status == "Critical")    return 0;
    if (status == "Offline")     return 1;
    if (status == "Degraded")    return 2;
    if (status == "Warning")     return 3;
    if (status == "Operational") return 4;
    return 5;
}

bool VoiceAlertManager::shouldAlert(const QString& status) const
{
    if (status == "Critical")    return m_alertCritical;
    if (status == "Degraded")    return m_alertDegraded;
    if (status == "Warning")     return m_alertWarning;
    if (status == "Offline")     return m_alertOffline;
    return false; // Operational and Unknown don't trigger alerts
}

void VoiceAlertManager::speak(const QString& text)
{
    if (m_ttsEngine.isEmpty()) {
        // No TTS engine - log the alert text to console
        qDebug() << "[VoiceAlert] (no TTS engine) Would say:" << text;
        return;
    }

    // Clean up any previous process
    if (m_ttsProcess) {
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(500);
        delete m_ttsProcess;
        m_ttsProcess = nullptr;
    }

    m_isSpeaking = true;
    m_ttsProcess = new QProcess(this);

    connect(m_ttsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VoiceAlertManager::onSpeechFinished);

    // Build arguments based on the TTS engine
    QStringList args;
    QString engine = QFileInfo(m_ttsEngine).fileName();

    if (engine == "espeak-ng" || engine == "espeak") {
        // Modern espeak-ng / espeak with a clear voice
        args << "-v" << "en"         // English voice
             << "-s" << "160"        // Speed (words per minute) - clear pace
             << "-p" << "50"         // Pitch (0-99) - natural mid-range
             << "-a" << "180"        // Amplitude (0-200) - strong volume
             << text;
    } else if (engine == "say") {
        // macOS 'say' command
        args << "-v" << "Samantha"   // Modern macOS voice
             << "-r" << "180"        // Rate
             << text;
    } else if (engine == "spd-say") {
        // speech-dispatcher
        args << "-w"                 // Wait for speech to finish
             << "-r" << "10"         // Rate (-100 to +100)
             << text;
    } else {
        args << text;
    }

    qDebug() << "[VoiceAlert] Speaking:" << m_ttsEngine << args;
    m_ttsProcess->start(m_ttsEngine, args);

    if (!m_ttsProcess->waitForStarted(2000)) {
        qWarning() << "[VoiceAlert] Failed to start TTS process:" << m_ttsProcess->errorString();
        m_isSpeaking = false;
        delete m_ttsProcess;
        m_ttsProcess = nullptr;
    }
}

QString VoiceAlertManager::findTtsEngine() const
{
    // Search for available TTS engines in order of preference
    QStringList candidates;
    candidates << "espeak-ng"    // Modern, widely available
               << "espeak"       // Classic fallback
               << "spd-say"      // speech-dispatcher
               << "say";         // macOS built-in

    for (const QString& cmd : candidates) {
        QProcess testProc;
        testProc.start("which", QStringList() << cmd);
        testProc.waitForFinished(2000);

        if (testProc.exitCode() == 0) {
            QString path = QString::fromUtf8(testProc.readAllStandardOutput()).trimmed();
            if (!path.isEmpty()) {
                return path;
            }
        }
    }

    return QString();
}
