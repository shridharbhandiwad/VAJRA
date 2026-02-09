#ifndef THEMEMANAGER_H
#define THEMEMANAGER_H

#include <QObject>
#include <QColor>
#include <QString>

/**
 * AppTheme - Enumerates the available application themes.
 */
enum class AppTheme {
    Dark,
    Light
};

/**
 * ThemeManager - Singleton that manages the application theme.
 *
 * Provides:
 *   - Dark / Light theme switching
 *   - QSS stylesheet loading and application
 *   - Color accessors for custom-painted widgets (Component, SubComponent, etc.)
 *   - Theme persistence via QSettings
 *   - Signal emission on theme change so widgets can repaint
 */
class ThemeManager : public QObject
{
    Q_OBJECT

public:
    static ThemeManager& instance();

    AppTheme currentTheme() const { return m_theme; }
    bool isDark() const { return m_theme == AppTheme::Dark; }
    void setTheme(AppTheme theme);
    void toggleTheme();

    /// Load the QSS for the current theme and apply it to qApp
    void applyTheme();

    /// Return the full QSS string for the current theme
    QString themeStyleSheet() const;

    // ── Background colours ──────────────────────────────────
    QColor windowBackground() const;
    QColor panelBackground() const;
    QColor cardBackground() const;
    QColor canvasBackground() const;
    QColor inputBackground() const;

    // ── Text colours ────────────────────────────────────────
    QColor primaryText() const;
    QColor secondaryText() const;
    QColor mutedText() const;
    QColor invertedText() const;

    // ── Border colours ──────────────────────────────────────
    QColor borderColor() const;
    QColor borderLight() const;
    QColor borderSubtle() const;

    // ── Accent colours (consistent across themes) ───────────
    QColor accentPrimary() const { return QColor("#00BCD4"); }
    QColor accentSecondary() const { return QColor("#00897B"); }
    QColor accentTertiary() const { return QColor("#0097A7"); }
    QColor accentSuccess() const { return QColor("#4CAF50"); }
    QColor accentWarning() const { return QColor("#FF9800"); }
    QColor accentDanger() const { return QColor("#F44336"); }

    // ── Component painting colours ──────────────────────────
    QColor componentBackground() const;
    QColor componentHeaderOverlay() const;
    QColor componentBorder() const;
    QColor componentShadow() const;
    QColor componentTextPrimary() const;
    QColor componentTextSecondary() const;
    QColor healthBarBackground() const;

    // ── SubComponent painting colours ───────────────────────
    QColor subcomponentBackground() const;
    QColor subcomponentBorder() const;
    QColor subcomponentText() const;

    // ── Connection painting colours ─────────────────────────
    QColor connectionLabelBackground() const;
    QColor connectionDefaultColor() const;

    // ── Analytics HTML styles ───────────────────────────────
    QString analyticsStyleBlock() const;
    QString analyticsComponentCardBg() const;
    QString analyticsComponentCardBorder() const;

    // ── Chart / EnlargedView painting colours ───────────────
    QColor chartBackground() const;
    QColor chartBorder() const;
    QColor chartGridLine() const;
    QColor chartAxisText() const;
    QColor chartPlaceholderText() const;

    // ── Stat card colours (EnlargedComponentView) ───────────
    QColor statCardBackground() const;
    QColor statCardBorder() const;
    QColor statCardLabel() const;
    QColor statCardValue() const;

    // ── Save / load preference ──────────────────────────────
    void savePreference() const;
    void loadPreference();

signals:
    void themeChanged(AppTheme newTheme);

private:
    ThemeManager();
    ~ThemeManager() = default;
    ThemeManager(const ThemeManager&) = delete;
    ThemeManager& operator=(const ThemeManager&) = delete;

    QString loadQssFile(const QString& path) const;

    AppTheme m_theme;
};

#endif // THEMEMANAGER_H
