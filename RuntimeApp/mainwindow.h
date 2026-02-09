#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QMap>
#include "canvas.h"
#include "analytics.h"
#include "messageserver.h"
#include "voicealertmanager.h"
#include "enlargedcomponentview.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void loadDesign();
    void onMessageReceived(const QString& componentId, const QString& color, qreal size);
    void onComponentLoaded(const QString& id, const QString& type);
    void onClientConnected();
    void onClientDisconnected();
    void toggleVoiceAlerts();
    void testVoice();
    
private:
    void setupUI();
    void autoLoadDesign();
    void createComponentTabs();
    void clearComponentTabs();
    
    // Tab widget for enlarged component views
    QTabWidget* m_tabWidget;
    
    // System overview tab contents
    Canvas* m_canvas;
    Analytics* m_analytics;
    
    // Per-component enlarged views (keyed by component ID)
    QMap<QString, EnlargedComponentView*> m_enlargedViews;
    
    MessageServer* m_messageServer;
    QLabel* m_statusLabel;
    int m_connectedClients;
    
    VoiceAlertManager* m_voiceAlertManager;
    QPushButton* m_voiceToggleBtn;
};

#endif // MAINWINDOW_H
