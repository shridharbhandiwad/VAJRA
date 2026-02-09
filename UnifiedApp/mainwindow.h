#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QTabWidget>
#include <QMap>
#include "logindialog.h"
#include "componentlist.h"
#include "canvas.h"
#include "analytics.h"
#include "messageserver.h"
#include "voicealertmanager.h"
#include "enlargedcomponentview.h"
#include "thememanager.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(UserRole userRole, const QString& username, QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void saveDesign();
    void loadDesign();
    void clearCanvas();
    void addNewComponentType();
    void onComponentAdded(const QString& id, const QString& typeId);
    void onMessageReceived(const QString& componentId, const QString& color, qreal size);
    void onComponentLoaded(const QString& id, const QString& typeId);
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
    
private:
    void setupUI();
    void setupDesignerMode();
    void setupRuntimeMode();
    void autoLoadDesign();
    void createComponentTabs();
    void clearComponentTabs();
    void updateThemeButtonText();
    void refreshCanvasBackground();
    
    UserRole m_userRole;
    QString m_username;
    
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
    
    // Tab widget for enlarged component views (Runtime mode)
    QTabWidget* m_tabWidget;
    QMap<QString, EnlargedComponentView*> m_enlargedViews;
};

#endif // MAINWINDOW_H
