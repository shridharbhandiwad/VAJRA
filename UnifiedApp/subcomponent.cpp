#include "subcomponent.h"
#include "thememanager.h"
#include <QPainter>
#include <QDebug>

SubComponent::SubComponent(const QString& name, int index, QGraphicsItem* parent)
    : QGraphicsItem(parent)
    , m_name(name)
    , m_color(QColor("#4CAF50"))  // Default green (healthy)
    , m_health(100.0)
    , m_index(index)
{
    setFlag(QGraphicsItem::ItemIsSelectable, false);
    // Position is managed by parent Component
}

QRectF SubComponent::boundingRect() const
{
    return QRectF(0, 0, itemWidth(), itemHeight());
}

void SubComponent::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    
    ThemeManager& tm = ThemeManager::instance();
    
    painter->setRenderHint(QPainter::Antialiasing);
    
    qreal w = itemWidth();
    qreal h = itemHeight();
    
    // Background with rounded corners
    painter->setPen(QPen(tm.subcomponentBorder(), 1));
    painter->setBrush(tm.subcomponentBackground());
    painter->drawRoundedRect(0, 0, w, h, 4, 4);
    
    // Health indicator bar on the left
    qreal barWidth = 4;
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_color);
    painter->drawRoundedRect(0, 0, barWidth, h, 2, 2);
    
    // Sub-component name
    painter->setPen(tm.subcomponentText());
    painter->setFont(QFont("Inter", 9, QFont::Normal));
    QRectF textRect(barWidth + 6, 0, w - barWidth - 40, h);
    painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, m_name);
    
    // Health percentage on the right
    painter->setPen(m_color);
    painter->setFont(QFont("Inter", 9, QFont::Bold));
    QRectF healthRect(w - 38, 0, 34, h);
    painter->drawText(healthRect, Qt::AlignVCenter | Qt::AlignRight, 
                      QString("%1%").arg(qRound(m_health)));
    
    // Selection highlight (from parent's selection state)
    if (isSelected()) {
        painter->setPen(QPen(tm.accentPrimary(), 1, Qt::DashLine));
        painter->setBrush(Qt::NoBrush);
        painter->drawRoundedRect(0, 0, w, h, 4, 4);
    }
}

void SubComponent::setColor(const QColor& color)
{
    m_color = color;
    update();
}

void SubComponent::setHealth(qreal health)
{
    m_health = qBound(0.0, health, 100.0);
    update();
}

void SubComponent::setIndex(int index)
{
    m_index = index;
}

QPointF SubComponent::centerInParent() const
{
    return pos() + QPointF(itemWidth() / 2.0, itemHeight() / 2.0);
}
