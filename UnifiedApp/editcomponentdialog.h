#ifndef EDITCOMPONENTDIALOG_H
#define EDITCOMPONENTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include "component.h"

/**
 * EditComponentDialog - Modern dialog for editing existing components.
 * 
 * Features:
 * - Change component display name and label
 * - Add/remove subsystems (SubComponent items)
 * - Add/remove design subcomponents (Label/LineEdit/Button widgets)
 * - Export component to .cmp file
 * - Export design subcomponents to .subcmp file
 * - Beautiful, modern UI with consistent styling
 */
class EditComponentDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EditComponentDialog(Component* component, QWidget* parent = nullptr);
    
    // Get the updated component properties (returns true if changes were made)
    bool hasChanges() const { return m_hasChanges; }
    
private slots:
    void onAddSubsystem();
    void onRemoveSubsystem();
    void onAddDesignWidget();
    void onRemoveDesignWidget();
    void onExportComponent();
    void onExportSubcomponents();
    void onApplyClicked();
    void onCancelClicked();
    void validateForm();

private:
    void setupUI();
    void applyStyles();
    void loadComponentData();
    void applyChanges();
    
    Component* m_component;
    bool m_hasChanges;
    
    // UI Elements
    QLineEdit* m_nameEdit;
    QLineEdit* m_labelEdit;
    QLineEdit* m_typeIdEdit;
    
    // Subsystems management
    QListWidget* m_subsystemList;
    QPushButton* m_addSubsystemBtn;
    QPushButton* m_removeSubsystemBtn;
    QPushButton* m_exportComponentBtn;
    
    // Design subcomponents management
    QListWidget* m_designWidgetList;
    QPushButton* m_addDesignWidgetBtn;
    QPushButton* m_removeDesignWidgetBtn;
    QPushButton* m_exportSubcomponentsBtn;
    
    // Action buttons
    QPushButton* m_applyButton;
    QPushButton* m_cancelButton;
};

#endif // EDITCOMPONENTDIALOG_H
