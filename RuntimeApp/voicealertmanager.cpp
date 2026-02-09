#include "voicealertmanager.h"
#include <QDebug>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QColor>
#include <QDateTime>
#include <QCoreApplication>

VoiceAlertManager::VoiceAlertManager(QObject* parent)
    : QObject(parent)
    , m_queueTimer(new QTimer(this))
    , m_isSpeaking(false)
    , m_ttsProcess(nullptr)
    , m_strategy(NoStrategy)
    , m_watchdogTimer(new QTimer(this))
    , m_cooldownMs(15000)   // 15 second cooldown per component
    , m_muted(false)
    , m_alertCritical(true)
    , m_alertDegraded(true)
    , m_alertWarning(false)  // Off by default - too noisy
    , m_alertOffline(true)
{
    qDebug() << "=========================================";
    qDebug() << "[VoiceAlert] Initializing Voice Alert System";
    qDebug() << "=========================================";

    // Step 1: Find TTS engine (espeak-ng, espeak, spd-say, say)
    m_ttsEngine = findTtsEngine();
    if (m_ttsEngine.isEmpty()) {
        qWarning() << "[VoiceAlert] *** NO TTS ENGINE FOUND ***";
        qWarning() << "[VoiceAlert] Voice alerts require espeak-ng.";
        qWarning() << "[VoiceAlert] Install with: sudo apt-get install espeak-ng alsa-utils";
    } else {
        qDebug() << "[VoiceAlert] TTS engine:" << m_ttsEngine;
    }

    // Step 2: Find audio player (aplay, paplay, play)
    m_audioPlayer = findAudioPlayer();
    if (m_audioPlayer.isEmpty()) {
        qDebug() << "[VoiceAlert] No separate audio player found.";
        qDebug() << "[VoiceAlert] Install alsa-utils for best results: sudo apt-get install alsa-utils";
    } else {
        qDebug() << "[VoiceAlert] Audio player:" << m_audioPlayer;
    }

    // Step 3: Select the best available strategy
    m_strategy = detectBestStrategy();
    qDebug() << "[VoiceAlert] Selected strategy:" << strategyName();

    // Step 4: Log audio diagnostics
    logAudioDiagnostics();

    if (m_strategy == NoStrategy) {
        qWarning() << "[VoiceAlert] Voice alerts DISABLED - no TTS engine available.";
        qWarning() << "[VoiceAlert] Run: sudo apt-get install espeak-ng alsa-utils";
    } else {
        qDebug() << "[VoiceAlert] Voice alerts READY";
    }

    qDebug() << "[VoiceAlert] Diagnostic:" << diagnosticInfo();
    qDebug() << "=========================================";

    // Queue timer processes pending alerts at a steady rate
    m_queueTimer->setInterval(500);
    connect(m_queueTimer, &QTimer::timeout, this, &VoiceAlertManager::processQueue);
    m_queueTimer->start();

    // Watchdog timer to recover from hung TTS processes (single-shot)
    m_watchdogTimer->setSingleShot(true);
    m_watchdogTimer->setInterval(WATCHDOG_TIMEOUT_MS);
    connect(m_watchdogTimer, &QTimer::timeout, this, &VoiceAlertManager::onSpeechWatchdogTimeout);

    emit ttsStatusChanged(m_strategy != NoStrategy,
        m_strategy != NoStrategy ? m_ttsEngine : "Not available");
}

VoiceAlertManager::~VoiceAlertManager()
{
    m_watchdogTimer->stop();
    m_queueTimer->stop();

    if (m_ttsProcess) {
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(1000);
        delete m_ttsProcess;
    }

    cleanupWavFile();
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

void VoiceAlertManager::testVoice()
{
    qDebug() << "[VoiceAlert] === Voice Test Requested ===";
    qDebug() << "[VoiceAlert] Strategy:" << strategyName();
    qDebug() << "[VoiceAlert] TTS Engine:" << (m_ttsEngine.isEmpty() ? "NONE" : m_ttsEngine);
    qDebug() << "[VoiceAlert] Audio Player:" << (m_audioPlayer.isEmpty() ? "NONE" : m_audioPlayer);

    // Re-detect TTS in case it was installed after startup
    if (m_strategy == NoStrategy) {
        qDebug() << "[VoiceAlert] Re-scanning for TTS engines...";
        m_ttsEngine = findTtsEngine();
        m_audioPlayer = findAudioPlayer();
        m_strategy = detectBestStrategy();
        qDebug() << "[VoiceAlert] Re-detected strategy:" << strategyName();
    }

    if (m_strategy == NoStrategy) {
        qWarning() << "[VoiceAlert] Cannot test - no TTS engine available.";
        qWarning() << "[VoiceAlert] Install with: sudo apt-get install espeak-ng alsa-utils";
        return;
    }

    // Force unmute for test
    bool wasMuted = m_muted;
    m_muted = false;

    speak("Voice alert system test. Audio is working correctly.");

    // Restore mute state after a delay (the speech will be in progress)
    if (wasMuted) {
        QTimer::singleShot(5000, this, [this, wasMuted]() {
            m_muted = wasMuted;
        });
    }
}

bool VoiceAlertManager::isTtsAvailable() const
{
    return m_strategy != NoStrategy;
}

QString VoiceAlertManager::diagnosticInfo() const
{
    QStringList info;
    info << QString("Engine: %1").arg(m_ttsEngine.isEmpty() ? "NONE" : m_ttsEngine);
    info << QString("Player: %1").arg(m_audioPlayer.isEmpty() ? "NONE" : m_audioPlayer);
    info << QString("Strategy: %1").arg(strategyName());
    info << QString("Muted: %1").arg(m_muted ? "Yes" : "No");
    return info.join(" | ");
}

QString VoiceAlertManager::strategyName() const
{
    switch (m_strategy) {
        case ShellPipeline:   return "Pipeline (espeak|aplay)";
        case WavFilePlayback: return "WAV file + audio player";
        case DirectTTS:       return "Direct TTS";
        case NoStrategy:      return "None (disabled)";
    }
    return "Unknown";
}

// ---------------------------------------------------------------------------
// Private slots
// ---------------------------------------------------------------------------

void VoiceAlertManager::processQueue()
{
    if (m_isSpeaking || m_alertQueue.isEmpty() || m_muted) {
        return;
    }

    AlertEntry entry = m_alertQueue.takeFirst();

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
            QString stdOut = QString::fromUtf8(m_ttsProcess->readAllStandardOutput()).trimmed();
            if (!stdErr.isEmpty()) {
                qWarning() << "[VoiceAlert] stderr:" << stdErr;
            }
            if (!stdOut.isEmpty()) {
                qDebug() << "[VoiceAlert] stdout:" << stdOut;
            }
        }
    } else {
        qDebug() << "[VoiceAlert] Speech completed successfully";
    }

    cleanupWavFile();
    resetSpeakingState();
}

void VoiceAlertManager::onSpeechError(QProcess::ProcessError error)
{
    qWarning() << "[VoiceAlert] TTS process error:" << error;

    m_watchdogTimer->stop();

    if (m_ttsProcess) {
        qWarning() << "[VoiceAlert] Error string:" << m_ttsProcess->errorString();
    }

    cleanupWavFile();
    resetSpeakingState();
}

void VoiceAlertManager::onSpeechWatchdogTimeout()
{
    qWarning() << "[VoiceAlert] Watchdog timeout - killing hung process";

    if (m_ttsProcess) {
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(1000);
    }

    cleanupWavFile();
    resetSpeakingState();
}

// ---------------------------------------------------------------------------
// Speech output strategies
// ---------------------------------------------------------------------------

void VoiceAlertManager::speak(const QString& text)
{
    if (m_strategy == NoStrategy) {
        qDebug() << "[VoiceAlert] (no TTS) Would say:" << text;
        return;
    }

    // Clean up any previous process
    if (m_ttsProcess) {
        m_watchdogTimer->stop();
        m_ttsProcess->disconnect(this);
        m_ttsProcess->kill();
        m_ttsProcess->waitForFinished(500);
        m_ttsProcess->deleteLater();
        m_ttsProcess = nullptr;
    }
    cleanupWavFile();

    m_isSpeaking = true;
    m_ttsProcess = new QProcess(this);

    // Connect both finished and error signals for robust state management
    connect(m_ttsProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &VoiceAlertManager::onSpeechFinished);
    connect(m_ttsProcess, &QProcess::errorOccurred,
            this, &VoiceAlertManager::onSpeechError);

    // Start the watchdog timer before launching the process
    m_watchdogTimer->start();

    // Execute the selected strategy
    switch (m_strategy) {
        case ShellPipeline:
            speakWithShellPipeline(text);
            break;
        case WavFilePlayback:
            speakWithWavFile(text);
            break;
        case DirectTTS:
            speakDirect(text);
            break;
        default:
            qWarning() << "[VoiceAlert] No strategy available";
            m_watchdogTimer->stop();
            resetSpeakingState();
            return;
    }

    if (!m_ttsProcess->waitForStarted(5000)) {
        qWarning() << "[VoiceAlert] Failed to start TTS process:" << m_ttsProcess->errorString();
        qWarning() << "[VoiceAlert] Strategy was:" << strategyName();
        m_watchdogTimer->stop();
        resetSpeakingState();
    }
}

void VoiceAlertManager::speakWithShellPipeline(const QString& text)
{
    // Most reliable on Linux: generate WAV to stdout and pipe to audio player
    // This bypasses PulseAudio integration issues in espeak
    QString escaped = shellEscape(text);
    QString engine = QFileInfo(m_ttsEngine).fileName();
    QString player = QFileInfo(m_audioPlayer).fileName();

    QString cmd;
    if (engine == "espeak-ng" || engine == "espeak") {
        if (player == "aplay") {
            // aplay reads WAV from stdin natively
            cmd = QString("%1 --stdout -v en -s 160 -p 50 -a 200 '%2' 2>/dev/null | %3 -q 2>/dev/null")
                .arg(m_ttsEngine).arg(escaped).arg(m_audioPlayer);
        } else if (player == "paplay") {
            // paplay can also read WAV from stdin
            cmd = QString("%1 --stdout -v en -s 160 -p 50 -a 200 '%2' 2>/dev/null | %3 2>/dev/null")
                .arg(m_ttsEngine).arg(escaped).arg(m_audioPlayer);
        } else if (player == "play") {
            // SoX play needs -t wav - for stdin
            cmd = QString("%1 --stdout -v en -s 160 -p 50 -a 200 '%2' 2>/dev/null | %3 -q -t wav - 2>/dev/null")
                .arg(m_ttsEngine).arg(escaped).arg(m_audioPlayer);
        } else {
            cmd = QString("%1 --stdout -v en -s 160 -p 50 -a 200 '%2' 2>/dev/null | %3 2>/dev/null")
                .arg(m_ttsEngine).arg(escaped).arg(m_audioPlayer);
        }
    } else {
        // Generic pipeline
        cmd = QString("%1 --stdout '%2' 2>/dev/null | %3 2>/dev/null")
            .arg(m_ttsEngine).arg(escaped).arg(m_audioPlayer);
    }

    qDebug() << "[VoiceAlert] Pipeline cmd:" << cmd;
    m_ttsProcess->start("sh", QStringList() << "-c" << cmd);
}

void VoiceAlertManager::speakWithWavFile(const QString& text)
{
    // Generate a WAV file with espeak, then play it with a separate player
    m_currentWavFile = QString("/tmp/voice_alert_%1.wav")
        .arg(QDateTime::currentMSecsSinceEpoch());

    QString escaped = shellEscape(text);
    QString engine = QFileInfo(m_ttsEngine).fileName();
    QString player = QFileInfo(m_audioPlayer).fileName();

    QString genArgs;
    if (engine == "espeak-ng" || engine == "espeak") {
        genArgs = QString("-v en -s 160 -p 50 -a 200 -w '%1' '%2'")
            .arg(m_currentWavFile).arg(escaped);
    } else {
        genArgs = QString("-w '%1' '%2'")
            .arg(m_currentWavFile).arg(escaped);
    }

    QString playArgs;
    if (player == "aplay") {
        playArgs = QString("-q '%1'").arg(m_currentWavFile);
    } else if (player == "play") {
        playArgs = QString("-q '%1'").arg(m_currentWavFile);
    } else {
        playArgs = QString("'%1'").arg(m_currentWavFile);
    }

    // Chain: generate WAV, play it, clean up temp file
    QString cmd = QString("%1 %2 2>/dev/null && %3 %4 2>/dev/null; rm -f '%5'")
        .arg(m_ttsEngine).arg(genArgs)
        .arg(m_audioPlayer).arg(playArgs)
        .arg(m_currentWavFile);

    qDebug() << "[VoiceAlert] WAV+Play cmd:" << cmd;
    m_ttsProcess->start("sh", QStringList() << "-c" << cmd);
}

void VoiceAlertManager::speakDirect(const QString& text)
{
    // Simplest approach: let espeak handle audio output directly
    QStringList args;
    QString engine = QFileInfo(m_ttsEngine).fileName();

    if (engine == "espeak-ng" || engine == "espeak") {
        args << "-v" << "en"         // English voice
             << "-s" << "160"        // Speed (words per minute)
             << "-p" << "50"         // Pitch (0-99)
             << "-a" << "200"        // Amplitude (0-200) - maximum volume
             << text;
    } else if (engine == "say") {
        // macOS 'say' command
        args << "-v" << "Samantha"
             << "-r" << "180"
             << text;
    } else if (engine == "spd-say") {
        // speech-dispatcher
        args << "-w"                 // Wait for speech to finish
             << "-r" << "10"
             << text;
    } else {
        args << text;
    }

    qDebug() << "[VoiceAlert] Direct cmd:" << m_ttsEngine << args;
    m_ttsProcess->start(m_ttsEngine, args);
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

void VoiceAlertManager::cleanupWavFile()
{
    if (!m_currentWavFile.isEmpty()) {
        QFile::remove(m_currentWavFile);
        m_currentWavFile.clear();
    }
}

QString VoiceAlertManager::shellEscape(const QString& text) const
{
    // Escape single quotes for use inside single-quoted shell strings
    QString escaped = text;
    escaped.replace("'", "'\\''");
    return escaped;
}

void VoiceAlertManager::logAudioDiagnostics() const
{
    // List available sound devices using aplay -l
    QProcess deviceCheck;
    deviceCheck.start("sh", QStringList() << "-c" << "aplay -l 2>/dev/null || echo 'aplay not available'");
    deviceCheck.waitForFinished(3000);
    if (deviceCheck.exitCode() == 0) {
        QString output = QString::fromUtf8(deviceCheck.readAllStandardOutput()).trimmed();
        if (output.contains("card")) {
            qDebug() << "[VoiceAlert] Audio devices detected (aplay -l shows cards)";
        } else if (output.contains("not available")) {
            qDebug() << "[VoiceAlert] aplay not available - install alsa-utils";
        } else {
            qWarning() << "[VoiceAlert] No audio cards detected by ALSA";
            qWarning() << "[VoiceAlert] aplay -l output:" << output;
        }
    }

    // Check PulseAudio status
    QProcess pulseCheck;
    pulseCheck.start("sh", QStringList() << "-c" << "pactl info 2>/dev/null | head -3 || echo 'PulseAudio not available'");
    pulseCheck.waitForFinished(3000);
    if (pulseCheck.exitCode() == 0) {
        QString output = QString::fromUtf8(pulseCheck.readAllStandardOutput()).trimmed();
        if (output.contains("Server Name")) {
            qDebug() << "[VoiceAlert] PulseAudio is running";
        } else {
            qDebug() << "[VoiceAlert] PulseAudio status:" << output.left(100);
        }
    }

    // Quick TTS engine version check
    if (!m_ttsEngine.isEmpty()) {
        QProcess versionCheck;
        versionCheck.start(m_ttsEngine, QStringList() << "--version");
        versionCheck.waitForFinished(2000);
        if (versionCheck.exitCode() == 0) {
            QString version = QString::fromUtf8(versionCheck.readAllStandardOutput()).trimmed();
            if (!version.isEmpty()) {
                qDebug() << "[VoiceAlert] TTS version:" << version.left(80);
            }
        }
    }
}

QString VoiceAlertManager::resolveHealthStatus(const QString& color) const
{
    // Normalize the color: use QColor to parse any valid color format,
    // then compare by the resolved RGB values.
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

    // Compare using RGB values (ignores alpha, case, and hex formatting)
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

VoiceAlertManager::SpeakStrategy VoiceAlertManager::detectBestStrategy() const
{
    if (m_ttsEngine.isEmpty()) {
        return NoStrategy;
    }

    QString engine = QFileInfo(m_ttsEngine).fileName();
    bool isEspeak = (engine == "espeak-ng" || engine == "espeak");

    // Strategy 1: Pipeline - most reliable, bypasses PulseAudio issues
    // Requires espeak + an audio player that can read WAV from stdin
    if (isEspeak && !m_audioPlayer.isEmpty()) {
        QString player = QFileInfo(m_audioPlayer).fileName();
        // aplay and paplay support reading WAV from stdin
        if (player == "aplay" || player == "paplay" || player == "play") {
            qDebug() << "[VoiceAlert] Strategy: ShellPipeline"
                     << "(" << m_ttsEngine << "--stdout |" << m_audioPlayer << ")";
            return ShellPipeline;
        }
    }

    // Strategy 2: WAV file + audio player
    // Requires espeak + any audio player
    if (isEspeak && !m_audioPlayer.isEmpty()) {
        qDebug() << "[VoiceAlert] Strategy: WavFilePlayback"
                 << "(" << m_ttsEngine << "-w file.wav &&" << m_audioPlayer << "file.wav)";
        return WavFilePlayback;
    }

    // Strategy 3: Direct TTS
    // espeak handles audio output itself (may fail with PulseAudio issues)
    qDebug() << "[VoiceAlert] Strategy: DirectTTS (" << m_ttsEngine << "directly)";
    return DirectTTS;
}

QString VoiceAlertManager::findTtsEngine() const
{
    // Method 1: Qt's built-in executable finder (searches PATH reliably)
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

    // Method 2: Check common absolute paths directly
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

    // Method 3: Legacy fallback using 'which' command
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

QString VoiceAlertManager::findAudioPlayer() const
{
    // Search for audio players in order of preference:
    // aplay (ALSA) - most reliable on Linux, bypasses PulseAudio
    // paplay (PulseAudio) - works when PulseAudio is running
    // play (SoX) - versatile but less common
    QStringList candidates;
    candidates << "aplay"      // ALSA player (alsa-utils package)
               << "paplay"     // PulseAudio player
               << "play";      // SoX player

    // Method 1: QStandardPaths
    for (const QString& cmd : candidates) {
        QString path = QStandardPaths::findExecutable(cmd);
        if (!path.isEmpty()) {
            qDebug() << "[VoiceAlert] Found audio player via QStandardPaths:" << path;
            return path;
        }
    }

    // Method 2: Common absolute paths
    QStringList absolutePaths;
    absolutePaths << "/usr/bin/aplay"
                  << "/usr/local/bin/aplay"
                  << "/usr/bin/paplay"
                  << "/usr/local/bin/paplay"
                  << "/usr/bin/play"
                  << "/usr/local/bin/play";

    for (const QString& absPath : absolutePaths) {
        QFileInfo fi(absPath);
        if (fi.exists() && fi.isExecutable()) {
            qDebug() << "[VoiceAlert] Found audio player via absolute path:" << absPath;
            return absPath;
        }
    }

    // Method 3: Legacy 'which' fallback
    for (const QString& cmd : candidates) {
        QProcess testProc;
        testProc.start("which", QStringList() << cmd);
        testProc.waitForFinished(2000);

        if (testProc.exitCode() == 0) {
            QString path = QString::fromUtf8(testProc.readAllStandardOutput()).trimmed();
            if (!path.isEmpty()) {
                qDebug() << "[VoiceAlert] Found audio player via 'which':" << path;
                return path;
            }
        }
    }

    return QString();
}
