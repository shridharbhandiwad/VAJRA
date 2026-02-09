#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include "componentlist.h"
#include "canvas.h"
#include "analytics.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();
    
private slots:
    void saveDesign();
    void loadDesign();
    void clearCanvas();
    void onComponentAdded(const QString& id, ComponentType type);
    void onSubComponentAdded(const QString& parentId, SubComponentType subType);
    void onDropRejected(const QString& reason);
    
private:
    void setupUI();
    QString getComponentTypeName(ComponentType type);
    
    ComponentList* m_componentList;
    Canvas* m_canvas;
    Analytics* m_analytics;
};

#endif // MAINWINDOW_H
