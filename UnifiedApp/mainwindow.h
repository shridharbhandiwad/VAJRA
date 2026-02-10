#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QMap>
#include "componentlist.h"
#include "canvas.h"
#include "analytics.h"
#include "messageserver.h"
#include "voicealertmanager.h"
#include "enlargedcomponentview.h"
#include "thememanager.h"
#include "designsubcomponent.h"
#include "userrole.h"
#include <QJsonObject>

/**
 * MainWindow - Unified application window that combines Designer and Runtime
 * features into a single cohesive interface.
 *
 * Features:
 *   - Component drag-drop design (from ComponentList to Canvas)
 *   - Sub-component (Label/LineEdit/Button) drag-drop with resize inside components
 *   - Connection drawing between components
 *   - Real-time health monitoring via MessageServer
 *   - Voice alerts for critical health states
 *   - Enlarged per-component views with health trend charts
 *   - Analytics panel
 *   - Save/Load design files
 *   - Dark/Light theme switching
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(const QString& username, UserRole role, QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void saveDesign();
    void loadDesign();
    void clearCanvas();
    void addNewComponentType();
    void importComponent();
    void importSubcomponents();
    void onComponentAdded(const QString& id, const QString& typeId);
    void onComponentLoaded(const QString& id, const QString& typeId);
    void onDesignSubComponentAdded(const QString& parentId, SubComponentType subType);
    void onDropRejected(const QString& reason);
    void onMessageReceived(const QString& componentId, const QString& color, qreal size);
    void onSubsystemHealthReceived(const QString& componentId, const QString& subsystemName,
                                   const QString& color, qreal health);
    void onTelemetryReceived(const QString& componentId, const QJsonObject& telemetry);
    void onClientConnected();
    void onClientDisconnected();
    void toggleVoiceAlerts();
    void testVoice();
    
    // Connection mode slots
    void toggleConnectionMode();
    void onConnectionTypeChanged(int index);
    void onModeChanged(CanvasMode mode);
    
    // Theme slots
    void onThemeToggle();
    void onThemeChanged(AppTheme theme);
    
    // Logout slot
    void onLogout();

signals:
    void logoutRequested();
    
private:
    void setupUI();
    void applyRoleRestrictions();
    void autoLoadDesign();
    void createComponentTabs();
    void clearComponentTabs();
    void addComponentTab(Component* comp);
    void updateThemeButtonText();
    void refreshCanvasBackground();
    
    QString m_username;
    UserRole m_role;
    
    ComponentList* m_componentList;
    Canvas* m_canvas;
    Analytics* m_analytics;
    MessageServer* m_messageServer;
    QLabel* m_statusLabel;
    QLabel* m_userLabel;
    int m_connectedClients;
    
    VoiceAlertManager* m_voiceAlertManager;
    QPushButton* m_voiceToggleBtn;
    
    // Connection UI
    QPushButton* m_connectBtn;
    QComboBox* m_connectionTypeCombo;
    
    // Theme UI
    QPushButton* m_themeToggleBtn;
    
    // Logout button
    QPushButton* m_logoutBtn;
    
    // Design toolbar buttons (Save/Clear/AddType hidden for User role; Load visible for all)
    QPushButton* m_saveBtn;
    QPushButton* m_loadBtn;
    QPushButton* m_clearBtn;
    QPushButton* m_addTypeBtn;
    QPushButton* m_importComponentBtn;
    QPushButton* m_importSubcomponentsBtn;
    
    // Toolbar QActions (needed to properly hide toolbar items)
    QAction* m_saveBtnAction;
    QAction* m_loadBtnAction;
    QAction* m_clearBtnAction;
    QAction* m_addTypeBtnAction;
    QAction* m_importComponentBtnAction;
    QAction* m_importSubcomponentsBtnAction;
    QAction* m_connectBtnAction;
    QAction* m_connectionTypeComboAction;
    QAction* m_voiceToggleBtnAction;
    QAction* m_testVoiceBtnAction;
    // Toolbar separators (hidden for User role to keep toolbar clean)
    QAction* m_designSep1;   // after Load
    QAction* m_designSep2;   // after AddType
    QAction* m_connectSep;   // after connection controls
    QAction* m_statusSep;    // after status label
    QAction* m_voiceSep;     // after voice controls
    
    // Left panel (hidden for User role)
    QWidget* m_leftPanel;
    
    // Tab widget for enlarged component views
    QTabWidget* m_tabWidget;
    QMap<QString, EnlargedComponentView*> m_enlargedViews;
};

#endif // MAINWINDOW_H
