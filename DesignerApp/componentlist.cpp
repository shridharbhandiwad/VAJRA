#include "componentlist.h"
#include "component.h"
#include "subcomponent.h"
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QIcon>
#include <QPixmap>
#include <QFileInfo>

ComponentList::ComponentList(QWidget* parent)
    : QListWidget(parent)
{
    // Set larger icon size for better visibility
    setIconSize(QSize(48, 48));
    
    // --- Main components (radar subsystems) ---
    addComponentItem("Antenna", ComponentType::Antenna);
    addComponentItem("Power System", ComponentType::PowerSystem);
    addComponentItem("Liquid Cooling Unit", ComponentType::LiquidCoolingUnit);
    addComponentItem("Communication System", ComponentType::CommunicationSystem);
    addComponentItem("Radar Computer", ComponentType::RadarComputer);
    
    // --- Separator ---
    QListWidgetItem* separator = new QListWidgetItem("  ─── Sub-Components ───");
    separator->setFlags(Qt::NoItemFlags);   // not selectable / draggable
    separator->setBackground(QColor(36, 39, 46));
    separator->setForeground(QColor(120, 125, 135));
    QFont sepFont("Segoe UI", 8, QFont::Bold);
    separator->setFont(sepFont);
    addItem(separator);
    
    // --- Draggable sub-component types ---
    addSubComponentItem("Label");
    addSubComponentItem("LineEdit");
    addSubComponentItem("Button");
    
    setDragEnabled(true);
    setMaximumWidth(180);
}

void ComponentList::addComponentItem(const QString& name, ComponentType type)
{
    QListWidgetItem* item = new QListWidgetItem(name);
    
    // Load icon for the component
    QString dirName = Component::getSubsystemDirName(type);
    QString iconPath = QString("assets/subsystems/%1/%2_main.jpg").arg(dirName).arg(dirName);
    
    QFileInfo checkFile(iconPath);
    if (checkFile.exists() && checkFile.isFile()) {
        QPixmap pixmap(iconPath);
        if (!pixmap.isNull()) {
            QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            item->setIcon(QIcon(scaledPixmap));
        }
    } else {
        iconPath = QString("assets/subsystems/%1/%2_main.png").arg(dirName).arg(dirName);
        QFileInfo checkFilePng(iconPath);
        if (checkFilePng.exists() && checkFilePng.isFile()) {
            QPixmap pixmap(iconPath);
            if (!pixmap.isNull()) {
                QPixmap scaledPixmap = pixmap.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                item->setIcon(QIcon(scaledPixmap));
            }
        }
    }
    
    addItem(item);
}

void ComponentList::addSubComponentItem(const QString& name)
{
    QListWidgetItem* item = new QListWidgetItem(name);
    
    // Mark as a sub-component item via UserRole data
    item->setData(Qt::UserRole, QStringLiteral("subcomponent"));
    
    // Generate a small coloured icon for each sub-component type
    QPixmap pix(48, 48);
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setRenderHint(QPainter::Antialiasing);
    
    if (name == "Label") {
        p.setPen(QPen(QColor(100, 105, 115), 1));
        p.setBrush(QColor(52, 56, 63));
        p.drawRoundedRect(4, 12, 40, 24, 3, 3);
        p.setPen(QColor(220, 222, 228));
        p.setFont(QFont("Segoe UI", 8));
        p.drawText(QRect(4, 12, 40, 24), Qt::AlignCenter, "Abc");
    } else if (name == "LineEdit") {
        p.setPen(QPen(QColor(90, 95, 105), 1));
        p.setBrush(QColor(40, 42, 50));
        p.drawRoundedRect(4, 12, 40, 24, 3, 3);
        p.setPen(QPen(QColor(33, 150, 243), 2));
        p.drawLine(6, 34, 42, 34);
        p.setPen(QColor(140, 145, 155));
        p.setFont(QFont("Segoe UI", 7));
        p.drawText(QRect(4, 12, 40, 22), Qt::AlignCenter, "Edit");
    } else if (name == "Button") {
        QLinearGradient grad(0, 12, 0, 36);
        grad.setColorAt(0, QColor(30, 110, 220));
        grad.setColorAt(1, QColor(20, 85, 180));
        p.setPen(QPen(QColor(15, 70, 160), 1));
        p.setBrush(grad);
        p.drawRoundedRect(4, 12, 40, 24, 4, 4);
        p.setPen(Qt::white);
        p.setFont(QFont("Segoe UI", 7, QFont::Bold));
        p.drawText(QRect(4, 12, 40, 24), Qt::AlignCenter, "Btn");
    }
    p.end();
    
    item->setIcon(QIcon(pix));
    addItem(item);
}

void ComponentList::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void ComponentList::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        return;
    }
    
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    
    // Use itemAt to get the item where the drag started (avoids dragging the separator)
    QListWidgetItem* item = itemAt(m_dragStartPosition);
    if (!item || !(item->flags() & Qt::ItemIsEnabled)) {
        return;
    }
    
    QDrag* drag = new QDrag(this);
    QMimeData* mimeData = new QMimeData;
    
    // Prefix sub-component items so the canvas can distinguish them
    if (item->data(Qt::UserRole).toString() == "subcomponent") {
        mimeData->setText("subcomponent:" + item->text());
    } else {
        mimeData->setText(item->text());
    }
    
    drag->setMimeData(mimeData);
    drag->exec(Qt::CopyAction);
}
