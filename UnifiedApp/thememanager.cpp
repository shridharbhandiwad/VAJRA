#include "thememanager.h"
#include <QApplication>
#include <QFile>
#include <QSettings>
#include <QDebug>

// ═══════════════════════════════════════════════════════════════
//  Singleton
// ═══════════════════════════════════════════════════════════════

ThemeManager& ThemeManager::instance()
{
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager()
    : QObject(nullptr)
    , m_theme(AppTheme::Dark)
{
    loadPreference();
}

// ═══════════════════════════════════════════════════════════════
//  Theme switching
// ═══════════════════════════════════════════════════════════════

void ThemeManager::setTheme(AppTheme theme)
{
    if (m_theme == theme) return;
    m_theme = theme;
    applyTheme();
    savePreference();
    emit themeChanged(m_theme);
}

void ThemeManager::toggleTheme()
{
    setTheme(m_theme == AppTheme::Dark ? AppTheme::Light : AppTheme::Dark);
}

void ThemeManager::applyTheme()
{
    QString qss = themeStyleSheet();
    if (qApp) {
        qApp->setStyleSheet(qss);
    }
    qDebug() << "[ThemeManager] Applied theme:" << (isDark() ? "Dark" : "Light");
}

QString ThemeManager::themeStyleSheet() const
{
    QString fileName = isDark() ? "styles_dark.qss" : "styles_light.qss";

    // Try resource path first
    QString qss = loadQssFile(":/styles/" + fileName);
    if (!qss.isEmpty()) return qss;

    // Try relative path
    qss = loadQssFile(fileName);
    if (!qss.isEmpty()) return qss;

    qWarning() << "[ThemeManager] Could not load QSS file:" << fileName;
    return QString();
}

QString ThemeManager::loadQssFile(const QString& path) const
{
    QFile file(path);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        return QLatin1String(file.readAll());
    }
    return QString();
}

// ═══════════════════════════════════════════════════════════════
//  Persistence
// ═══════════════════════════════════════════════════════════════

void ThemeManager::savePreference() const
{
    QSettings settings;
    settings.setValue("theme", isDark() ? "dark" : "light");
}

void ThemeManager::loadPreference()
{
    QSettings settings;
    QString saved = settings.value("theme", "dark").toString();
    m_theme = (saved == "light") ? AppTheme::Light : AppTheme::Dark;
}

// ═══════════════════════════════════════════════════════════════
//  Background colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::windowBackground() const
{
    return isDark() ? QColor("#12141a") : QColor("#F0F2F5");
}

QColor ThemeManager::panelBackground() const
{
    return isDark() ? QColor(24, 27, 33) : QColor("#FFFFFF");
}

QColor ThemeManager::cardBackground() const
{
    return isDark() ? QColor(36, 39, 46) : QColor("#F7F8FA");
}

QColor ThemeManager::canvasBackground() const
{
    return isDark() ? QColor(14, 16, 21) : QColor("#E4E8ED");
}

QColor ThemeManager::inputBackground() const
{
    return isDark() ? QColor(28, 30, 38) : QColor("#FFFFFF");
}

// ═══════════════════════════════════════════════════════════════
//  Text colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::primaryText() const
{
    return isDark() ? QColor("#e8eaed") : QColor("#1A1D23");
}

QColor ThemeManager::secondaryText() const
{
    return isDark() ? QColor("#9aa0a6") : QColor("#5A6070");
}

QColor ThemeManager::mutedText() const
{
    return isDark() ? QColor("#5f6368") : QColor("#8E95A5");
}

QColor ThemeManager::invertedText() const
{
    return isDark() ? QColor("#1A1D23") : QColor("#FFFFFF");
}

// ═══════════════════════════════════════════════════════════════
//  Border colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::borderColor() const
{
    return isDark() ? QColor(55, 60, 70) : QColor("#D0D5DD");
}

QColor ThemeManager::borderLight() const
{
    return isDark() ? QColor(255, 255, 255, 15) : QColor("#E4E8ED");
}

QColor ThemeManager::borderSubtle() const
{
    return isDark() ? QColor(255, 255, 255, 10) : QColor("#EAEDF2");
}

// ═══════════════════════════════════════════════════════════════
//  Component painting colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::componentBackground() const
{
    return isDark() ? QColor(28, 30, 38) : QColor("#FFFFFF");
}

QColor ThemeManager::componentHeaderOverlay() const
{
    return isDark() ? QColor(0, 0, 0, 50) : QColor(0, 0, 0, 20);
}

QColor ThemeManager::componentBorder() const
{
    return isDark() ? QColor(55, 60, 70) : QColor("#C8CDD5");
}

QColor ThemeManager::componentShadow() const
{
    return isDark() ? QColor(0, 0, 0, 50) : QColor(0, 0, 0, 25);
}

QColor ThemeManager::componentTextPrimary() const
{
    return isDark() ? QColor(230, 232, 237) : QColor("#1A1D23");
}

QColor ThemeManager::componentTextSecondary() const
{
    return isDark() ? QColor(120, 125, 135) : QColor("#6B7280");
}

QColor ThemeManager::healthBarBackground() const
{
    return isDark() ? QColor(20, 22, 28) : QColor("#E0E4EA");
}

// ═══════════════════════════════════════════════════════════════
//  SubComponent painting colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::subcomponentBackground() const
{
    return isDark() ? QColor(36, 39, 46) : QColor("#F3F4F6");
}

QColor ThemeManager::subcomponentBorder() const
{
    return isDark() ? QColor(55, 60, 70) : QColor("#D1D5DB");
}

QColor ThemeManager::subcomponentText() const
{
    return isDark() ? QColor(200, 200, 210) : QColor("#374151");
}

// ═══════════════════════════════════════════════════════════════
//  Connection painting colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::connectionLabelBackground() const
{
    return isDark() ? QColor(24, 26, 31, 220) : QColor(255, 255, 255, 230);
}

QColor ThemeManager::connectionDefaultColor() const
{
    return isDark() ? QColor(100, 180, 220) : QColor(0, 137, 181);
}

// ═══════════════════════════════════════════════════════════════
//  Analytics HTML styles
// ═══════════════════════════════════════════════════════════════

QString ThemeManager::analyticsStyleBlock() const
{
    if (isDark()) {
        return "<style>"
               "body { color: #c4c7cc; font-family: 'Consolas', 'Monaco', monospace; font-size: 11px; }"
               ".header { color: #00BCD4; font-size: 12px; font-weight: bold; letter-spacing: 2px; }"
               ".subheader { color: #9aa0a6; font-size: 10px; margin-top: 4px; }"
               ".component { margin: 8px 0; padding: 8px; background: #1c1e26; border-radius: 4px; border-left: 3px solid #3a3f4b; }"
               ".component-name { color: #e8eaed; font-weight: bold; }"
               ".stat { color: #9aa0a6; font-size: 10px; }"
               ".operational { color: #4CAF50; }"
               ".warning { color: #FFC107; }"
               ".degraded { color: #FF9800; }"
               ".critical { color: #F44336; }"
               ".offline { color: #9E9E9E; }"
               ".count { color: #00BCD4; font-weight: bold; }"
               "</style>";
    } else {
        return "<style>"
               "body { color: #374151; font-family: 'Consolas', 'Monaco', monospace; font-size: 11px; }"
               ".header { color: #0097A7; font-size: 12px; font-weight: bold; letter-spacing: 2px; }"
               ".subheader { color: #6B7280; font-size: 10px; margin-top: 4px; }"
               ".component { margin: 8px 0; padding: 8px; background: #F3F4F6; border-radius: 4px; border-left: 3px solid #D1D5DB; }"
               ".component-name { color: #1A1D23; font-weight: bold; }"
               ".stat { color: #6B7280; font-size: 10px; }"
               ".operational { color: #16A34A; }"
               ".warning { color: #D97706; }"
               ".degraded { color: #EA580C; }"
               ".critical { color: #DC2626; }"
               ".offline { color: #9CA3AF; }"
               ".count { color: #0097A7; font-weight: bold; }"
               "</style>";
    }
}

QString ThemeManager::analyticsComponentCardBg() const
{
    return isDark() ? "#1c1e26" : "#F3F4F6";
}

QString ThemeManager::analyticsComponentCardBorder() const
{
    return isDark() ? "#3a3f4b" : "#D1D5DB";
}

// ═══════════════════════════════════════════════════════════════
//  Chart / EnlargedView painting colours
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::chartBackground() const
{
    return isDark() ? QColor(18, 20, 26) : QColor("#FFFFFF");
}

QColor ThemeManager::chartBorder() const
{
    return isDark() ? QColor(55, 60, 70, 120) : QColor(200, 205, 215, 180);
}

QColor ThemeManager::chartGridLine() const
{
    return isDark() ? QColor(50, 54, 65) : QColor(220, 225, 232);
}

QColor ThemeManager::chartAxisText() const
{
    return isDark() ? QColor(140, 143, 150) : QColor(107, 114, 128);
}

QColor ThemeManager::chartPlaceholderText() const
{
    return isDark() ? QColor(100, 104, 115) : QColor(156, 163, 175);
}

// ═══════════════════════════════════════════════════════════════
//  Stat card colours (EnlargedComponentView)
// ═══════════════════════════════════════════════════════════════

QColor ThemeManager::statCardBackground() const
{
    return isDark() ? QColor(18, 20, 26, 230) : QColor("#F7F8FA");
}

QColor ThemeManager::statCardBorder() const
{
    return isDark() ? QColor(255, 255, 255, 15) : QColor("#E0E4EA");
}

QColor ThemeManager::statCardLabel() const
{
    return isDark() ? QColor("#6c717a") : QColor("#8E95A5");
}

QColor ThemeManager::statCardValue() const
{
    return isDark() ? QColor("#e8eaed") : QColor("#1A1D23");
}
