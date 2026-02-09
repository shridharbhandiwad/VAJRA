#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLabel>
#include "logindialog.h"
#include "componentlist.h"
#include "canvas.h"
#include "analytics.h"
#include "messageserver.h"
#include "voicealertmanager.h"

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
    
private:
    void setupUI();
    void setupDesignerMode();
    void setupRuntimeMode();
    void autoLoadDesign();
    
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
};

#endif // MAINWINDOW_H
