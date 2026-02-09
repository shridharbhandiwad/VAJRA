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
    MainWindow(const QString& username, QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void saveDesign();
    void loadDesign();
    void clearCanvas();
    void addNewComponentType();
    void onComponentAdded(const QString& id, const QString& typeId);
    void onComponentLoaded(const QString& id, const QString& typeId);
    void onDesignSubComponentAdded(const QString& parentId, SubComponentType subType);
    void onDropRejected(const QString& reason);
    void onMessageReceived(const QString& componentId, const QString& color, qreal size);
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
    void autoLoadDesign();
    void createComponentTabs();
    void clearComponentTabs();
    void addComponentTab(Component* comp);
    void updateThemeButtonText();
    void refreshCanvasBackground();
    
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
    
    // Tab widget for enlarged component views
    QTabWidget* m_tabWidget;
    QMap<QString, EnlargedComponentView*> m_enlargedViews;
};

#endif // MAINWINDOW_H
