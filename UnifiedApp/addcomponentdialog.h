#ifndef ADDCOMPONENTDIALOG_H
#define ADDCOMPONENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QTextEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include "componentregistry.h"

/**
 * AddComponentDialog - Modern dialog for adding new component types to the registry.
 * 
 * This dialog allows users to define entirely new component types at runtime:
 * - Component name, label, and description
 * - Image file selection
 * - Subsystems definition
 * - Health protocol selection (TCP, UDP, WebSocket, MQTT)
 * - Port configuration
 * - Visual shape selection
 * - Category assignment
 * 
 * No code changes are needed - the new component is saved to components.json
 * and immediately available in the component list.
 */
class AddComponentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddComponentDialog(QWidget* parent = nullptr);
    
    ComponentDefinition getComponentDefinition() const;

private slots:
    void onBrowseImage();
    void onAddClicked();
    void onNameChanged(const QString& text);
    void validateForm();

private:
    void setupUI();
    void applyStyles();
    QString generateTypeId(const QString& displayName) const;
    
    // Form fields
    QLineEdit* m_nameEdit;
    QLineEdit* m_labelEdit;
    QTextEdit* m_descriptionEdit;
    QLineEdit* m_imagePathEdit;
    QPushButton* m_browseBtn;
    QTextEdit* m_subsystemsEdit;
    QComboBox* m_protocolCombo;
    QSpinBox* m_portSpin;
    QComboBox* m_categoryCombo;
    QComboBox* m_shapeCombo;
    
    // Buttons
    QPushButton* m_addButton;
    QPushButton* m_cancelButton;
    
    // Status
    QLabel* m_statusLabel;
    QLabel* m_previewLabel;
    
    // Result
    ComponentDefinition m_result;
};

#endif // ADDCOMPONENTDIALOG_H
