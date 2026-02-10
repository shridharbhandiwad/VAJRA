#ifndef COMPONENTLIST_H
#define COMPONENTLIST_H

#include <QListWidget>
#include <QMouseEvent>
#include <QWidget>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

/**
 * ComponentItemWidget - Custom widget for component list items with delete button
 */
class ComponentItemWidget : public QWidget
{
    Q_OBJECT
    
public:
    explicit ComponentItemWidget(const QString& displayName, const QString& typeId, QWidget* parent = nullptr);
    QString getTypeId() const { return m_typeId; }
    QString getDisplayName() const { return m_displayName; }
    
signals:
    void deleteRequested(const QString& typeId);
    
private:
    QString m_displayName;
    QString m_typeId;
    QLabel* m_nameLabel;
    QPushButton* m_deleteBtn;
};

/**
 * ComponentList - Dynamically populated component list driven by the ComponentRegistry.
 * 
 * This list auto-populates from the registry. When new components are added
 * to the registry (via JSON or UI), the list updates automatically.
 * No code changes needed to display new component types.
 * 
 * Also includes draggable sub-component widget types (Label, LineEdit, Button)
 * that can be dropped inside parent components on the canvas.
 */
class ComponentList : public QListWidget
{
    Q_OBJECT
    
public:
    explicit ComponentList(QWidget* parent = nullptr);
    
    // Refresh list from registry (called when components are added/removed)
    void refreshFromRegistry();
    
protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    
private slots:
    void onDeleteComponent(const QString& typeId);
    
private:
    void addComponentItem(const QString& displayName, const QString& typeId, 
                          const QString& imageDir, const QColor& iconColor);
    void addSubComponentItem(const QString& name);
    QPoint m_dragStartPosition;
};

#endif // COMPONENTLIST_H
