#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include "canvas.h"
#include "analytics.h"
#include "messageserver.h"
#include "voicealertmanager.h"

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
    
private:
    void setupUI();
    void autoLoadDesign();
    
    Canvas* m_canvas;
    Analytics* m_analytics;
    MessageServer* m_messageServer;
    QLabel* m_statusLabel;
    int m_connectedClients;
    
    VoiceAlertManager* m_voiceAlertManager;
    QPushButton* m_voiceToggleBtn;
};

#endif // MAINWINDOW_H
