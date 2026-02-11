#include "editcomponentdialog.h"
#include "componentregistry.h"
#include "thememanager.h"
#include "canvas.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>

EditComponentDialog::EditComponentDialog(Component* component, Canvas* canvas, QWidget* parent)
    : QDialog(parent)
    , m_component(component)
    , m_canvas(canvas)
    , m_hasChanges(false)
{
    setWindowTitle("Edit Component");
    setModal(true);
    resize(700, 650);
    
    setupUI();
    applyStyles();
    loadComponentData();
}

void EditComponentDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(24, 24, 24, 24);
    
    // ===== Header =====
    QLabel* titleLabel = new QLabel("Edit Component");
    titleLabel->setObjectName("dialogTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    // ===== Basic Properties Section =====
    QGroupBox* propsGroup = new QGroupBox("Component Properties");
    propsGroup->setObjectName("modernGroupBox");
    QVBoxLayout* propsLayout = new QVBoxLayout(propsGroup);
    propsLayout->setSpacing(12);
    
    // Type ID (read-only)
    QHBoxLayout* typeIdLayout = new QHBoxLayout();
    QLabel* typeIdLabel = new QLabel("Type ID:");
    typeIdLabel->setMinimumWidth(100);
    m_typeIdEdit = new QLineEdit();
    m_typeIdEdit->setReadOnly(true);
    m_typeIdEdit->setObjectName("readOnlyField");
    typeIdLayout->addWidget(typeIdLabel);
    typeIdLayout->addWidget(m_typeIdEdit);
    propsLayout->addLayout(typeIdLayout);
    
    // Display Name
    QHBoxLayout* nameLayout = new QHBoxLayout();
    QLabel* nameLabel = new QLabel("Display Name:");
    nameLabel->setMinimumWidth(100);
    m_nameEdit = new QLineEdit();
    m_nameEdit->setPlaceholderText("Enter component display name");
    connect(m_nameEdit, &QLineEdit::textChanged, this, &EditComponentDialog::validateForm);
    nameLayout->addWidget(nameLabel);
    nameLayout->addWidget(m_nameEdit);
    propsLayout->addLayout(nameLayout);
    
    // Label
    QHBoxLayout* labelLayout = new QHBoxLayout();
    QLabel* labelLabel = new QLabel("Label:");
    labelLabel->setMinimumWidth(100);
    m_labelEdit = new QLineEdit();
    m_labelEdit->setPlaceholderText("Short label (e.g., ANT, PWR)");
    m_labelEdit->setMaxLength(6);
    connect(m_labelEdit, &QLineEdit::textChanged, this, &EditComponentDialog::validateForm);
    labelLayout->addWidget(labelLabel);
    labelLayout->addWidget(m_labelEdit);
    propsLayout->addLayout(labelLayout);
    
    mainLayout->addWidget(propsGroup);
    
    // ===== Subsystems Section =====
    QGroupBox* subsystemGroup = new QGroupBox("Subsystems (Health Tracking)");
    subsystemGroup->setObjectName("modernGroupBox");
    QVBoxLayout* subsystemLayout = new QVBoxLayout(subsystemGroup);
    subsystemLayout->setSpacing(12);
    
    QLabel* subsystemDesc = new QLabel("Manage subsystem components for health monitoring:");
    subsystemDesc->setObjectName("descriptionLabel");
    subsystemDesc->setWordWrap(true);
    subsystemLayout->addWidget(subsystemDesc);
    
    m_subsystemList = new QListWidget();
    m_subsystemList->setObjectName("modernListWidget");
    m_subsystemList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_subsystemList->setMinimumHeight(120);
    subsystemLayout->addWidget(m_subsystemList);
    
    QHBoxLayout* subsystemBtnLayout = new QHBoxLayout();
    m_addSubsystemBtn = new QPushButton("➕ Add Subsystem");
    m_addSubsystemBtn->setObjectName("modernButton");
    m_removeSubsystemBtn = new QPushButton("➖ Remove Selected");
    m_removeSubsystemBtn->setObjectName("modernButton");
    m_exportComponentBtn = new QPushButton("💾 Export Component (.cmp)");
    m_exportComponentBtn->setObjectName("modernButton");
    
    connect(m_addSubsystemBtn, &QPushButton::clicked, this, &EditComponentDialog::onAddSubsystem);
    connect(m_removeSubsystemBtn, &QPushButton::clicked, this, &EditComponentDialog::onRemoveSubsystem);
    connect(m_exportComponentBtn, &QPushButton::clicked, this, &EditComponentDialog::onExportComponent);
    
    subsystemBtnLayout->addWidget(m_addSubsystemBtn);
    subsystemBtnLayout->addWidget(m_removeSubsystemBtn);
    subsystemBtnLayout->addStretch();
    subsystemBtnLayout->addWidget(m_exportComponentBtn);
    subsystemLayout->addLayout(subsystemBtnLayout);
    
    mainLayout->addWidget(subsystemGroup);
    
    // ===== Design Widgets Section =====
    QGroupBox* widgetGroup = new QGroupBox("Design Widgets");
    widgetGroup->setObjectName("modernGroupBox");
    QVBoxLayout* widgetLayout = new QVBoxLayout(widgetGroup);
    widgetLayout->setSpacing(12);
    
    QLabel* widgetDesc = new QLabel("Manage design widgets (Label, LineEdit, Button):");
    widgetDesc->setObjectName("descriptionLabel");
    widgetDesc->setWordWrap(true);
    widgetLayout->addWidget(widgetDesc);
    
    m_designWidgetList = new QListWidget();
    m_designWidgetList->setObjectName("modernListWidget");
    m_designWidgetList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_designWidgetList->setMinimumHeight(100);
    widgetLayout->addWidget(m_designWidgetList);
    
    QHBoxLayout* widgetBtnLayout = new QHBoxLayout();
    m_addDesignWidgetBtn = new QPushButton("➕ Add Widget");
    m_addDesignWidgetBtn->setObjectName("modernButton");
    m_removeDesignWidgetBtn = new QPushButton("➖ Remove Selected");
    m_removeDesignWidgetBtn->setObjectName("modernButton");
    m_exportSubcomponentsBtn = new QPushButton("💾 Export Widgets (.subcmp)");
    m_exportSubcomponentsBtn->setObjectName("modernButton");
    
    connect(m_addDesignWidgetBtn, &QPushButton::clicked, this, &EditComponentDialog::onAddDesignWidget);
    connect(m_removeDesignWidgetBtn, &QPushButton::clicked, this, &EditComponentDialog::onRemoveDesignWidget);
    connect(m_exportSubcomponentsBtn, &QPushButton::clicked, this, &EditComponentDialog::onExportSubcomponents);
    
    widgetBtnLayout->addWidget(m_addDesignWidgetBtn);
    widgetBtnLayout->addWidget(m_removeDesignWidgetBtn);
    widgetBtnLayout->addStretch();
    widgetBtnLayout->addWidget(m_exportSubcomponentsBtn);
    widgetLayout->addLayout(widgetBtnLayout);
    
    mainLayout->addWidget(widgetGroup);
    
    // ===== Action Buttons =====
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    m_applyButton = new QPushButton("Apply Changes");
    m_applyButton->setObjectName("primaryButton");
    m_applyButton->setMinimumWidth(140);
    m_applyButton->setMinimumHeight(38);
    connect(m_applyButton, &QPushButton::clicked, this, &EditComponentDialog::onApplyClicked);
    
    m_cancelButton = new QPushButton("Cancel");
    m_cancelButton->setObjectName("secondaryButton");
    m_cancelButton->setMinimumWidth(100);
    m_cancelButton->setMinimumHeight(38);
    connect(m_cancelButton, &QPushButton::clicked, this, &EditComponentDialog::onCancelClicked);
    
    buttonLayout->addWidget(m_cancelButton);
    buttonLayout->addWidget(m_applyButton);
    
    mainLayout->addLayout(buttonLayout);
    
    validateForm();
}

void EditComponentDialog::applyStyles()
{
    ThemeManager& tm = ThemeManager::instance();
    
    QString stylesheet = QString(R"(
        QDialog {
            background-color: %1;
        }
        
        QLabel#dialogTitle {
            font-size: 22px;
            font-weight: bold;
            color: %2;
            padding: 8px;
        }
        
        QLabel#descriptionLabel {
            font-size: 12px;
            color: %3;
            padding: 4px 0;
        }
        
        QGroupBox#modernGroupBox {
            font-weight: bold;
            font-size: 13px;
            color: %4;
            border: 2px solid %5;
            border-radius: 8px;
            margin-top: 12px;
            padding-top: 12px;
        }
        
        QGroupBox#modernGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            left: 16px;
            padding: 0 8px;
        }
        
        QLineEdit {
            padding: 8px 12px;
            border: 2px solid %6;
            border-radius: 6px;
            background-color: %7;
            color: %8;
            font-size: 13px;
        }
        
        QLineEdit:focus {
            border-color: %9;
        }
        
        QLineEdit#readOnlyField {
            background-color: %10;
            color: %11;
        }
        
        QListWidget#modernListWidget {
            border: 2px solid %12;
            border-radius: 6px;
            background-color: %13;
            color: %14;
            padding: 4px;
            font-size: 13px;
        }
        
        QListWidget#modernListWidget::item {
            padding: 6px 10px;
            border-radius: 4px;
            margin: 2px;
        }
        
        QListWidget#modernListWidget::item:selected {
            background-color: %15;
            color: white;
        }
        
        QListWidget#modernListWidget::item:hover {
            background-color: %16;
        }
        
        QPushButton#modernButton {
            padding: 8px 16px;
            border: 2px solid %17;
            border-radius: 6px;
            background-color: %18;
            color: %19;
            font-size: 12px;
            font-weight: bold;
        }
        
        QPushButton#modernButton:hover {
            background-color: %20;
            border-color: %21;
        }
        
        QPushButton#modernButton:pressed {
            background-color: %22;
        }
        
        QPushButton#primaryButton {
            padding: 10px 20px;
            border: none;
            border-radius: 6px;
            background-color: %23;
            color: white;
            font-size: 14px;
            font-weight: bold;
        }
        
        QPushButton#primaryButton:hover {
            background-color: %24;
        }
        
        QPushButton#primaryButton:pressed {
            background-color: %25;
        }
        
        QPushButton#primaryButton:disabled {
            background-color: %26;
            color: %27;
        }
        
        QPushButton#secondaryButton {
            padding: 10px 20px;
            border: 2px solid %28;
            border-radius: 6px;
            background-color: transparent;
            color: %29;
            font-size: 14px;
            font-weight: bold;
        }
        
        QPushButton#secondaryButton:hover {
            background-color: %30;
        }
    )")
    .arg(tm.isDark() ? "#1e2329" : "#f5f7fa")      // 1: dialog bg
    .arg(tm.isDark() ? "#e8eaed" : "#1a1f2e")      // 2: title color
    .arg(tm.isDark() ? "#9ca3af" : "#6b7280")      // 3: description
    .arg(tm.isDark() ? "#d1d5db" : "#374151")      // 4: group title
    .arg(tm.isDark() ? "#374151" : "#d1d5db")      // 5: group border
    .arg(tm.isDark() ? "#4b5563" : "#d1d5db")      // 6: input border
    .arg(tm.isDark() ? "#2d3748" : "#ffffff")      // 7: input bg
    .arg(tm.isDark() ? "#e8eaed" : "#1f2937")      // 8: input text
    .arg("#4f46e5")                                 // 9: input focus border
    .arg(tm.isDark() ? "#1f2937" : "#f3f4f6")      // 10: readonly bg
    .arg(tm.isDark() ? "#9ca3af" : "#6b7280")      // 11: readonly text
    .arg(tm.isDark() ? "#4b5563" : "#d1d5db")      // 12: list border
    .arg(tm.isDark() ? "#2d3748" : "#ffffff")      // 13: list bg
    .arg(tm.isDark() ? "#e8eaed" : "#1f2937")      // 14: list text
    .arg("#4f46e5")                                 // 15: list selected
    .arg(tm.isDark() ? "#374151" : "#f3f4f6")      // 16: list hover
    .arg(tm.isDark() ? "#4b5563" : "#d1d5db")      // 17: button border
    .arg(tm.isDark() ? "#374151" : "#f9fafb")      // 18: button bg
    .arg(tm.isDark() ? "#e8eaed" : "#374151")      // 19: button text
    .arg(tm.isDark() ? "#4b5563" : "#f3f4f6")      // 20: button hover
    .arg(tm.isDark() ? "#6b7280" : "#9ca3af")      // 21: button hover border
    .arg(tm.isDark() ? "#1f2937" : "#e5e7eb")      // 22: button pressed
    .arg("#4f46e5")                                 // 23: primary bg
    .arg("#4338ca")                                 // 24: primary hover
    .arg("#3730a3")                                 // 25: primary pressed
    .arg(tm.isDark() ? "#374151" : "#d1d5db")      // 26: primary disabled
    .arg(tm.isDark() ? "#6b7280" : "#9ca3af")      // 27: primary disabled text
    .arg(tm.isDark() ? "#4b5563" : "#9ca3af")      // 28: secondary border
    .arg(tm.isDark() ? "#e8eaed" : "#374151")      // 29: secondary text
    .arg(tm.isDark() ? "#374151" : "#f3f4f6");     // 30: secondary hover
    
    setStyleSheet(stylesheet);
}

void EditComponentDialog::loadComponentData()
{
    if (!m_component) return;
    
    // Load basic properties
    m_typeIdEdit->setText(m_component->getId());
    m_nameEdit->setText(m_component->getDisplayName());
    m_labelEdit->setText(m_component->getLabel());
    
    // Load subsystems
    m_subsystemList->clear();
    QList<SubComponent*> subComponents = m_component->getSubComponents();
    for (SubComponent* sub : subComponents) {
        m_subsystemList->addItem(sub->getName());
    }
    
    // Load design widgets
    m_designWidgetList->clear();
    QList<DesignSubComponent*> designWidgets = m_component->getDesignSubComponents();
    for (DesignSubComponent* widget : designWidgets) {
        QString widgetText = QString("%1: %2")
            .arg(DesignSubComponent::typeToString(widget->getType()))
            .arg(widget->getText());
        m_designWidgetList->addItem(widgetText);
    }
}

void EditComponentDialog::onAddSubsystem()
{
    bool ok;
    QString name = QInputDialog::getText(this, "Add Subsystem",
                                         "Enter subsystem name:",
                                         QLineEdit::Normal,
                                         "", &ok);
    if (ok && !name.isEmpty()) {
        m_subsystemList->addItem(name);
        validateForm();
    }
}

void EditComponentDialog::onRemoveSubsystem()
{
    QListWidgetItem* item = m_subsystemList->currentItem();
    if (item) {
        delete item;
        validateForm();
    }
}

void EditComponentDialog::onAddDesignWidget()
{
    QStringList widgetTypes;
    widgetTypes << "Label" << "LineEdit" << "Button";
    
    bool ok;
    QString type = QInputDialog::getItem(this, "Add Design Widget",
                                         "Select widget type:",
                                         widgetTypes, 0, false, &ok);
    if (!ok) return;
    
    QString text = QInputDialog::getText(this, "Add Design Widget",
                                         "Enter widget text:",
                                         QLineEdit::Normal,
                                         "", &ok);
    if (ok) {
        QString widgetText = QString("%1: %2").arg(type, text.isEmpty() ? "(empty)" : text);
        m_designWidgetList->addItem(widgetText);
        validateForm();
    }
}

void EditComponentDialog::onRemoveDesignWidget()
{
    QListWidgetItem* item = m_designWidgetList->currentItem();
    if (item) {
        delete item;
        validateForm();
    }
}

void EditComponentDialog::onExportComponent()
{
    if (!m_component) return;
    
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export Component",
                                                    m_component->getId() + ".cmp",
                                                    "Component Files (*.cmp)");
    if (fileName.isEmpty()) return;
    
    // Build JSON object
    QJsonObject json;
    json["type"] = "component";
    json["version"] = "1.0";
    json["typeId"] = m_component->getTypeId();
    json["id"] = m_component->getId();
    json["displayName"] = m_component->getDisplayName();
    json["label"] = m_component->getLabel();
    json["color"] = m_component->getColor().name();
    json["size"] = m_component->getSize();
    json["userWidth"] = m_component->getUserWidth();
    json["userHeight"] = m_component->getUserHeight();
    json["x"] = m_component->pos().x();
    json["y"] = m_component->pos().y();
    
    // Export subsystems
    QJsonArray subsystems;
    QList<SubComponent*> subComponents = m_component->getSubComponents();
    for (SubComponent* sub : subComponents) {
        QJsonObject subObj;
        subObj["name"] = sub->getName();
        subObj["health"] = sub->getHealth();
        subObj["color"] = sub->getColor().name();
        subsystems.append(subObj);
    }
    json["subsystems"] = subsystems;
    
    // Export design widgets
    QJsonArray widgets;
    QList<DesignSubComponent*> designWidgets = m_component->getDesignSubComponents();
    for (DesignSubComponent* widget : designWidgets) {
        QJsonObject widgetObj;
        widgetObj["type"] = DesignSubComponent::typeToString(widget->getType());
        widgetObj["text"] = widget->getText();
        widgetObj["x"] = widget->pos().x();
        widgetObj["y"] = widget->pos().y();
        widgetObj["width"] = widget->getWidth();
        widgetObj["height"] = widget->getHeight();
        widgets.append(widgetObj);
    }
    json["designWidgets"] = widgets;
    
    // Write to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(json);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Export Successful",
                                QString("Component exported to:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Export Failed",
                            QString("Could not write to file:\n%1").arg(fileName));
    }
}

void EditComponentDialog::onExportSubcomponents()
{
    if (!m_component) return;
    
    QList<DesignSubComponent*> widgets = m_component->getDesignSubComponents();
    if (widgets.isEmpty()) {
        QMessageBox::information(this, "No Widgets",
                                "This component has no design widgets to export.");
        return;
    }
    
    QString fileName = QFileDialog::getSaveFileName(this,
                                                    "Export Design Widgets",
                                                    m_component->getId() + "_widgets.subcmp",
                                                    "Subcomponent Files (*.subcmp)");
    if (fileName.isEmpty()) return;
    
    // Build JSON array
    QJsonArray widgetsArray;
    for (DesignSubComponent* widget : widgets) {
        QJsonObject widgetObj;
        widgetObj["type"] = DesignSubComponent::typeToString(widget->getType());
        widgetObj["text"] = widget->getText();
        widgetObj["x"] = widget->pos().x();
        widgetObj["y"] = widget->pos().y();
        widgetObj["width"] = widget->getWidth();
        widgetObj["height"] = widget->getHeight();
        widgetsArray.append(widgetObj);
    }
    
    QJsonObject json;
    json["type"] = "subcomponents";
    json["version"] = "1.0";
    json["widgets"] = widgetsArray;
    
    // Write to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(json);
        file.write(doc.toJson(QJsonDocument::Indented));
        file.close();
        QMessageBox::information(this, "Export Successful",
                                QString("Design widgets exported to:\n%1").arg(fileName));
    } else {
        QMessageBox::warning(this, "Export Failed",
                            QString("Could not write to file:\n%1").arg(fileName));
    }
}

void EditComponentDialog::onApplyClicked()
{
    applyChanges();
    m_hasChanges = true;
    accept();
}

void EditComponentDialog::onCancelClicked()
{
    reject();
}

void EditComponentDialog::validateForm()
{
    bool valid = !m_nameEdit->text().trimmed().isEmpty() &&
                 !m_labelEdit->text().trimmed().isEmpty();
    m_applyButton->setEnabled(valid);
}

void EditComponentDialog::applyChanges()
{
    if (!m_component) return;
    
    // Update display name and label (per-component customization)
    QString newDisplayName = m_nameEdit->text().trimmed();
    QString newLabel = m_labelEdit->text().trimmed();
    
    if (!newDisplayName.isEmpty()) {
        m_component->setDisplayName(newDisplayName);
    }
    if (!newLabel.isEmpty()) {
        m_component->setLabel(newLabel);
    }
    
    // Update subsystems
    // First, remove all existing subsystems
    // Important: Clean up connections before removing subcomponents to avoid crashes
    while (m_component->subComponentCount() > 0) {
        SubComponent* sub = m_component->getSubComponents()[0];
        if (m_canvas && sub) {
            m_canvas->removeConnectionsInvolvingSubComponent(sub);
        }
        m_component->removeSubComponent(0);
    }
    
    // Add new subsystems from the list
    for (int i = 0; i < m_subsystemList->count(); ++i) {
        QString name = m_subsystemList->item(i)->text();
        m_component->addSubComponent(name);
    }
    
    // Note: Design widgets are managed differently - we'll keep the existing ones
    // Adding/removing design widgets dynamically requires more complex logic
    // as they are separate QGraphicsItems. This would be a good future enhancement.
}

