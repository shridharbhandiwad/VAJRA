#include "voicealertmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QStandardPaths>
#include <QColor>

VoiceAlertManager::VoiceAlertManager(QObject* parent)
    : QObject(parent)
    , m_queueTimer(new QTimer(this))
    , m_isSpeaking(false)
    , m_ttsProcess(nullptr)
    , m_watchdogTimer(new QTimer(this))
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

    // Watchdog timer to recover from hung TTS processes (single-shot)
    m_watchdogTimer->setSingleShot(true);
    m_watchdogTimer->setInterval(WATCHDOG_TIMEOUT_MS);
    connect(m_watchdogTimer, &QTimer::timeout, this, &VoiceAlertManager::onSpeechWatchdogTimeout);
}

VoiceAlertManager::~VoiceAlertManager()
{
    m_watchdogTimer->stop();

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

    // Build the spoken text: "Antenna, System Status Critical, 45 percent"
    int healthRounded = qRound(entry.healthPercent);
    QString spokenText = QString("%1, System Status %2, %3 percent")
                             .arg(entry.componentName)
                             .arg(entry.status)
                             .arg(healthRounded);

    qDebug() << "[VoiceAlert] ALERT:" << entry.componentName
             << "-" << spokenText;

    speak(spokenText);

    emit alertTriggered(entry.componentId, entry.status, entry.healthPercent);
}

void VoiceAlertManager::onSpeechFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_UNUSED(exitStatus)

    m_watchdogTimer->stop();

    if (exitCode != 0) {
        qWarning() << "[VoiceAlert] TTS process exited with code:" << exitCode;
        if (m_ttsProcess) {
            QString stdErr = QString::fromUtf8(m_ttsProcess->readAllStandardError()).trimmed();
            if (!stdErr.isEmpty()) {
                qWarning() << "[VoiceAlert] TTS stderr:" << stdErr;
            }
        }
    }

    resetSpeakingState();
}

void VoiceAlertManager::onSpeechError(QProcess::ProcessError error)
{
    qWarning() << "[VoiceAlert] TTS process error:" << error;

    m_watchdogTimer->stop();

    if (m_ttsProcess) {
        qWarning() << "[VoiceAlert] TTS error string:" << m_ttsProcess->errorString();
    }

    resetSpeakingState();
}

void VoiceAlertManager::onSpeechWatchdogTimeout()
{
    qWarning() << "[VoiceAlert] TTS process watchdog timeout - killing hung process";

    if (m_ttsProcess) {
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(1000);
    }

    resetSpeakingState();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void VoiceAlertManager::resetSpeakingState()
{
    m_isSpeaking = false;

    if (m_ttsProcess) {
        m_ttsProcess->deleteLater();
        m_ttsProcess = nullptr;
    }
}

QString VoiceAlertManager::resolveHealthStatus(const QString& color) const
{
    // Normalize the color: use QColor to parse any valid color format,
    // then compare by the resolved RGB values. This handles:
    //   - "#00FF00", "#00ff00", "#0f0" (hex variants)
    //   - "red", "green", "yellow" (named colors)
    //   - "rgb(255,0,0)" (CSS-like, if QColor supports it)
    QColor qc(color);

    if (!qc.isValid()) {
        // Fallback: try raw string matching
        QString c = color.toLower().trimmed();
        if (c == "#00ff00" || c == "green")  return "Operational";
        if (c == "#ffff00" || c == "yellow") return "Warning";
        if (c == "#ffa500" || c == "orange") return "Degraded";
        if (c == "#ff0000" || c == "red")    return "Critical";
        if (c == "#808080" || c == "gray" || c == "grey") return "Offline";
        qDebug() << "[VoiceAlert] Unrecognized color (invalid QColor):" << color;
        return "Unknown";
    }

    // Compare using RGB values (ignores alpha, case, and hex formatting differences)
    int r = qc.red();
    int g = qc.green();
    int b = qc.blue();

    // Green - Operational (#00FF00)
    if (g >= 200 && r < 80 && b < 80)
        return "Operational";

    // Red - Critical (#FF0000)
    if (r >= 200 && g < 80 && b < 80)
        return "Critical";

    // Yellow - Warning (#FFFF00)
    if (r >= 200 && g >= 200 && b < 80)
        return "Warning";

    // Orange - Degraded (#FFA500)
    if (r >= 200 && g >= 100 && g <= 200 && b < 80)
        return "Degraded";

    // Gray - Offline (#808080)
    if (qAbs(r - g) < 30 && qAbs(g - b) < 30 && r >= 80 && r <= 180)
        return "Offline";

    // Exact match fallback for precision
    QString c = color.toLower().trimmed();
    if (c == "#00ff00") return "Operational";
    if (c == "#ffff00") return "Warning";
    if (c == "#ffa500") return "Degraded";
    if (c == "#ff0000") return "Critical";
    if (c == "#808080") return "Offline";

    qDebug() << "[VoiceAlert] Unrecognized color:" << color
             << "RGB(" << r << g << b << ")";
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
        m_watchdogTimer->stop();
        m_ttsProcess->disconnect(this); // Disconnect all signals to avoid stale callbacks
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(500);
        m_ttsProcess->deleteLater();
        m_ttsProcess = nullptr;
    }

    m_isSpeaking = true;
    m_ttsProcess = new QProcess(this);

    // Connect both finished and error signals for robust state management
    connect(m_ttsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VoiceAlertManager::onSpeechFinished);
    connect(m_ttsProcess, &QProcess::errorOccurred,
            this, &VoiceAlertManager::onSpeechError);

    // Build arguments based on the TTS engine
    QStringList args;
    QString engine = QFileInfo(m_ttsEngine).fileName();

    if (engine == "espeak-ng" || engine == "espeak") {
        // Modern espeak-ng / espeak with a clear voice
        args << "-v" << "en"         // English voice
             << "-s" << "160"        // Speed (words per minute) - clear pace
             << "-p" << "50"         // Pitch (0-99) - natural mid-range
             << "-a" << "200"        // Amplitude (0-200) - maximum volume
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

    // Start the watchdog timer before launching the process
    m_watchdogTimer->start();

    m_ttsProcess->start(m_ttsEngine, args);

    if (!m_ttsProcess->waitForStarted(3000)) {
        qWarning() << "[VoiceAlert] Failed to start TTS process:" << m_ttsProcess->errorString();
        qWarning() << "[VoiceAlert] TTS engine path:" << m_ttsEngine;
        m_watchdogTimer->stop();
        resetSpeakingState();
    }
}

QString VoiceAlertManager::findTtsEngine() const
{
    // Method 1: Use Qt's built-in executable finder (searches PATH reliably)
    QStringList candidates;
    candidates << "espeak-ng"    // Modern, widely available
               << "espeak"       // Classic fallback
               << "spd-say"      // speech-dispatcher
               << "say";         // macOS built-in

    for (const QString& cmd : candidates) {
        QString path = QStandardPaths::findExecutable(cmd);
        if (!path.isEmpty()) {
            qDebug() << "[VoiceAlert] Found TTS engine via QStandardPaths:" << path;
            return path;
        }
    }

    // Method 2: Check common absolute paths directly (handles cases where
    // the app's PATH doesn't include /usr/bin, /usr/local/bin, etc.)
    QStringList absolutePaths;
    absolutePaths << "/usr/bin/espeak-ng"
                  << "/usr/local/bin/espeak-ng"
                  << "/usr/bin/espeak"
                  << "/usr/local/bin/espeak"
                  << "/usr/bin/spd-say"
                  << "/usr/local/bin/spd-say"
                  << "/snap/bin/espeak-ng"
                  << "/snap/bin/espeak";

    for (const QString& absPath : absolutePaths) {
        QFileInfo fi(absPath);
        if (fi.exists() && fi.isExecutable()) {
            qDebug() << "[VoiceAlert] Found TTS engine via absolute path:" << absPath;
            return absPath;
        }
    }

    // Method 3: Legacy fallback using 'which' command (last resort)
    for (const QString& cmd : candidates) {
        QProcess testProc;
        testProc.start("which", QStringList() << cmd);
        testProc.waitForFinished(2000);

        if (testProc.exitCode() == 0) {
            QString path = QString::fromUtf8(testProc.readAllStandardOutput()).trimmed();
            if (!path.isEmpty()) {
                qDebug() << "[VoiceAlert] Found TTS engine via 'which':" << path;
                return path;
            }
        }
    }

    return QString();
}
