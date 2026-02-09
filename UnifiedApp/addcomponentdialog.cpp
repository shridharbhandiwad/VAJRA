#include "addcomponentdialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QScrollArea>
#include <QFrame>
#include <QGroupBox>
#include <QDebug>
#include <QRegularExpression>

AddComponentDialog::AddComponentDialog(QWidget* parent)
    : QDialog(parent)
{
    setupUI();
    applyStyles();
    
    setWindowTitle("Add New Component Type");
    setModal(true);
    setMinimumSize(560, 700);
    resize(560, 720);
}

void AddComponentDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(24, 20, 24, 20);
    mainLayout->setSpacing(16);
    
    // ========== HEADER ==========
    QLabel* titleLabel = new QLabel("ADD NEW COMPONENT TYPE", this);
    titleLabel->setObjectName("dialogTitle");
    titleLabel->setAlignment(Qt::AlignCenter);
    
    QLabel* subtitleLabel = new QLabel(
        "Define a new component type that will be available in the designer.\n"
        "No code changes required - it's saved to the component registry.", this);
    subtitleLabel->setObjectName("dialogSubtitle");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setWordWrap(true);
    
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(subtitleLabel);
    
    // ========== SCROLL AREA FOR FORM ==========
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setObjectName("formScrollArea");
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    
    QWidget* formWidget = new QWidget();
    QVBoxLayout* formLayout = new QVBoxLayout(formWidget);
    formLayout->setSpacing(14);
    formLayout->setContentsMargins(4, 4, 4, 4);
    
    // --- Identity Section ---
    QLabel* identityHeader = new QLabel("IDENTITY", this);
    identityHeader->setObjectName("sectionHeader");
    formLayout->addWidget(identityHeader);
    
    QFrame* identityFrame = new QFrame();
    identityFrame->setObjectName("formSection");
    QGridLayout* identityGrid = new QGridLayout(identityFrame);
    identityGrid->setSpacing(10);
    identityGrid->setContentsMargins(16, 14, 16, 14);
    
    QLabel* nameLabel = new QLabel("Component Name *");
    nameLabel->setObjectName("formLabel");
    m_nameEdit = new QLineEdit();
    m_nameEdit->setObjectName("formInput");
    m_nameEdit->setPlaceholderText("e.g., GPS Receiver, Signal Processor");
    
    QLabel* labelLabel = new QLabel("Short Label *");
    labelLabel->setObjectName("formLabel");
    m_labelEdit = new QLineEdit();
    m_labelEdit->setObjectName("formInput");
    m_labelEdit->setPlaceholderText("e.g., GPS, SIG (max 5 chars)");
    m_labelEdit->setMaxLength(5);
    
    QLabel* descLabel = new QLabel("Description");
    descLabel->setObjectName("formLabel");
    m_descriptionEdit = new QTextEdit();
    m_descriptionEdit->setObjectName("formTextArea");
    m_descriptionEdit->setPlaceholderText("Brief description of this component's function...");
    m_descriptionEdit->setMaximumHeight(60);
    
    identityGrid->addWidget(nameLabel, 0, 0);
    identityGrid->addWidget(m_nameEdit, 0, 1);
    identityGrid->addWidget(labelLabel, 1, 0);
    identityGrid->addWidget(m_labelEdit, 1, 1);
    identityGrid->addWidget(descLabel, 2, 0, Qt::AlignTop);
    identityGrid->addWidget(m_descriptionEdit, 2, 1);
    identityGrid->setColumnStretch(1, 1);
    formLayout->addWidget(identityFrame);
    
    // --- Image Section ---
    QLabel* imageHeader = new QLabel("IMAGE", this);
    imageHeader->setObjectName("sectionHeader");
    formLayout->addWidget(imageHeader);
    
    QFrame* imageFrame = new QFrame();
    imageFrame->setObjectName("formSection");
    QGridLayout* imageGrid = new QGridLayout(imageFrame);
    imageGrid->setSpacing(10);
    imageGrid->setContentsMargins(16, 14, 16, 14);
    
    QLabel* imageLabel = new QLabel("Image Directory");
    imageLabel->setObjectName("formLabel");
    
    QHBoxLayout* imagePathLayout = new QHBoxLayout();
    m_imagePathEdit = new QLineEdit();
    m_imagePathEdit->setObjectName("formInput");
    m_imagePathEdit->setPlaceholderText("Directory name under assets/subsystems/");
    
    m_browseBtn = new QPushButton("BROWSE");
    m_browseBtn->setObjectName("browseButton");
    m_browseBtn->setFixedWidth(80);
    imagePathLayout->addWidget(m_imagePathEdit);
    imagePathLayout->addWidget(m_browseBtn);
    
    m_previewLabel = new QLabel("No image selected");
    m_previewLabel->setObjectName("imagePreview");
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setMinimumHeight(48);
    
    imageGrid->addWidget(imageLabel, 0, 0);
    imageGrid->addLayout(imagePathLayout, 0, 1);
    imageGrid->addWidget(m_previewLabel, 1, 0, 1, 2);
    imageGrid->setColumnStretch(1, 1);
    formLayout->addWidget(imageFrame);
    
    // --- Subsystems Section ---
    QLabel* subsysHeader = new QLabel("SUBSYSTEMS", this);
    subsysHeader->setObjectName("sectionHeader");
    formLayout->addWidget(subsysHeader);
    
    QFrame* subsysFrame = new QFrame();
    subsysFrame->setObjectName("formSection");
    QVBoxLayout* subsysLayout = new QVBoxLayout(subsysFrame);
    subsysLayout->setSpacing(8);
    subsysLayout->setContentsMargins(16, 14, 16, 14);
    
    QLabel* subsysLabel = new QLabel("Subsystems (one per line)");
    subsysLabel->setObjectName("formLabel");
    m_subsystemsEdit = new QTextEdit();
    m_subsystemsEdit->setObjectName("formTextArea");
    m_subsystemsEdit->setPlaceholderText("e.g.,\nSignal Strength\nFrequency Accuracy\nSatellite Lock Count");
    m_subsystemsEdit->setMaximumHeight(80);
    
    subsysLayout->addWidget(subsysLabel);
    subsysLayout->addWidget(m_subsystemsEdit);
    formLayout->addWidget(subsysFrame);
    
    // --- Protocol & Config Section ---
    QLabel* configHeader = new QLabel("HEALTH PROTOCOL", this);
    configHeader->setObjectName("sectionHeader");
    formLayout->addWidget(configHeader);
    
    QFrame* configFrame = new QFrame();
    configFrame->setObjectName("formSection");
    QGridLayout* configGrid = new QGridLayout(configFrame);
    configGrid->setSpacing(10);
    configGrid->setContentsMargins(16, 14, 16, 14);
    
    QLabel* protocolLabel = new QLabel("Protocol *");
    protocolLabel->setObjectName("formLabel");
    m_protocolCombo = new QComboBox();
    m_protocolCombo->setObjectName("formCombo");
    m_protocolCombo->addItems(ComponentRegistry::instance().availableProtocols());
    
    QLabel* portLabel = new QLabel("Port");
    portLabel->setObjectName("formLabel");
    m_portSpin = new QSpinBox();
    m_portSpin->setObjectName("formSpin");
    m_portSpin->setRange(1024, 65535);
    m_portSpin->setValue(12345);
    
    QLabel* categoryLabel = new QLabel("Category");
    categoryLabel->setObjectName("formLabel");
    m_categoryCombo = new QComboBox();
    m_categoryCombo->setObjectName("formCombo");
    m_categoryCombo->setEditable(true);
    m_categoryCombo->addItems({"Sensor", "Infrastructure", "Network", "Processing", "Navigation", "Surveillance", "Custom"});
    
    QLabel* shapeLabel = new QLabel("Fallback Shape");
    shapeLabel->setObjectName("formLabel");
    m_shapeCombo = new QComboBox();
    m_shapeCombo->setObjectName("formCombo");
    m_shapeCombo->addItems({"rect", "ellipse", "hexagon", "diamond"});
    
    configGrid->addWidget(protocolLabel, 0, 0);
    configGrid->addWidget(m_protocolCombo, 0, 1);
    configGrid->addWidget(portLabel, 1, 0);
    configGrid->addWidget(m_portSpin, 1, 1);
    configGrid->addWidget(categoryLabel, 2, 0);
    configGrid->addWidget(m_categoryCombo, 2, 1);
    configGrid->addWidget(shapeLabel, 3, 0);
    configGrid->addWidget(m_shapeCombo, 3, 1);
    configGrid->setColumnStretch(1, 1);
    formLayout->addWidget(configFrame);
    
    formLayout->addStretch();
    scrollArea->setWidget(formWidget);
    mainLayout->addWidget(scrollArea, 1);
    
    // ========== STATUS ==========
    m_statusLabel = new QLabel("", this);
    m_statusLabel->setObjectName("formStatus");
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setVisible(false);
    mainLayout->addWidget(m_statusLabel);
    
    // ========== BUTTONS ==========
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(12);
    
    m_addButton = new QPushButton("ADD COMPONENT", this);
    m_addButton->setObjectName("addComponentBtn");
    m_addButton->setMinimumHeight(42);
    m_addButton->setEnabled(false);
    m_addButton->setCursor(Qt::PointingHandCursor);
    
    m_cancelButton = new QPushButton("CANCEL", this);
    m_cancelButton->setObjectName("cancelComponentBtn");
    m_cancelButton->setMinimumHeight(42);
    m_cancelButton->setCursor(Qt::PointingHandCursor);
    
    buttonLayout->addWidget(m_addButton);
    buttonLayout->addWidget(m_cancelButton);
    mainLayout->addLayout(buttonLayout);
    
    // ========== CONNECTIONS ==========
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddComponentDialog::onNameChanged);
    connect(m_nameEdit, &QLineEdit::textChanged, this, &AddComponentDialog::validateForm);
    connect(m_labelEdit, &QLineEdit::textChanged, this, &AddComponentDialog::validateForm);
    connect(m_browseBtn, &QPushButton::clicked, this, &AddComponentDialog::onBrowseImage);
    connect(m_addButton, &QPushButton::clicked, this, &AddComponentDialog::onAddClicked);
    connect(m_cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void AddComponentDialog::applyStyles()
{
    setStyleSheet(R"(
        QDialog {
            background: #1c1e26;
        }
        QLabel#dialogTitle {
            color: #e8eaed;
            font-size: 18px;
            font-weight: 600;
            letter-spacing: 2px;
            padding: 8px;
        }
        QLabel#dialogSubtitle {
            color: #9aa0a6;
            font-size: 11px;
            padding: 4px;
        }
        QLabel#sectionHeader {
            color: #00BCD4;
            font-size: 10px;
            font-weight: 700;
            letter-spacing: 2px;
            padding: 4px 2px;
        }
        QFrame#formSection {
            background: #24272e;
            border: 1px solid #3a3f4b;
            border-radius: 8px;
        }
        QLabel#formLabel {
            color: #9aa0a6;
            font-size: 11px;
            font-weight: 500;
            min-width: 110px;
        }
        QLineEdit#formInput, QTextEdit#formTextArea {
            background: #1c1e26;
            border: 1px solid #3a3f4b;
            border-radius: 6px;
            padding: 8px 12px;
            color: #e8eaed;
            font-size: 12px;
        }
        QLineEdit#formInput:focus, QTextEdit#formTextArea:focus {
            border: 1px solid #00BCD4;
        }
        QComboBox#formCombo {
            background: #1c1e26;
            border: 1px solid #3a3f4b;
            border-radius: 6px;
            padding: 8px 12px;
            color: #e8eaed;
            font-size: 12px;
            min-height: 20px;
        }
        QComboBox#formCombo:focus {
            border: 1px solid #00BCD4;
        }
        QComboBox#formCombo::drop-down {
            border: none;
            width: 24px;
        }
        QComboBox#formCombo QAbstractItemView {
            background: #24272e;
            border: 1px solid #3a3f4b;
            color: #e8eaed;
            selection-background-color: #00BCD4;
        }
        QSpinBox#formSpin {
            background: #1c1e26;
            border: 1px solid #3a3f4b;
            border-radius: 6px;
            padding: 8px 12px;
            color: #e8eaed;
            font-size: 12px;
            min-height: 20px;
        }
        QSpinBox#formSpin:focus {
            border: 1px solid #00BCD4;
        }
        QPushButton#browseButton {
            background: #2d313b;
            color: #e8eaed;
            border: 1px solid #3a3f4b;
            border-radius: 6px;
            padding: 8px;
            font-size: 10px;
            font-weight: 600;
            letter-spacing: 1px;
        }
        QPushButton#browseButton:hover {
            background: #34383f;
            border: 1px solid #00BCD4;
        }
        QLabel#imagePreview {
            color: #6c717a;
            font-size: 11px;
            padding: 8px;
            background: #1c1e26;
            border: 1px dashed #3a3f4b;
            border-radius: 6px;
        }
        QPushButton#addComponentBtn {
            background: #00BCD4;
            color: #ffffff;
            border: none;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            letter-spacing: 1px;
            padding: 12px 24px;
        }
        QPushButton#addComponentBtn:hover {
            background: #00ACC1;
        }
        QPushButton#addComponentBtn:pressed {
            background: #0097A7;
        }
        QPushButton#addComponentBtn:disabled {
            background: #2d313b;
            color: #6c717a;
        }
        QPushButton#cancelComponentBtn {
            background: #2d313b;
            color: #9aa0a6;
            border: 1px solid #3a3f4b;
            border-radius: 8px;
            font-size: 13px;
            font-weight: 600;
            letter-spacing: 1px;
            padding: 12px 24px;
        }
        QPushButton#cancelComponentBtn:hover {
            background: #34383f;
            border: 1px solid #4a4f5b;
        }
        QLabel#formStatus {
            color: #ef5350;
            font-size: 11px;
            padding: 8px;
            background: rgba(198, 40, 40, 0.1);
            border-radius: 6px;
        }
        QScrollArea#formScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            background: #1c1e26;
            width: 8px;
            border-radius: 4px;
        }
        QScrollBar::handle:vertical {
            background: #3a3f4b;
            border-radius: 4px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #4a4f5b;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");
}

void AddComponentDialog::onNameChanged(const QString& text)
{
    // Auto-generate label from name if label is empty or was auto-generated
    if (m_labelEdit->text().isEmpty() || m_labelEdit->text().length() <= 5) {
        QString autoLabel;
        QStringList words = text.split(' ', Qt::SkipEmptyParts);
        if (words.size() >= 2) {
            // Take first letter of each word
            for (const QString& word : words) {
                autoLabel += word.at(0).toUpper();
            }
        } else if (!text.isEmpty()) {
            autoLabel = text.left(4).toUpper();
        }
        if (autoLabel.length() > 5) autoLabel = autoLabel.left(5);
        m_labelEdit->setText(autoLabel);
    }
    
    // Auto-generate image dir
    if (m_imagePathEdit->text().isEmpty() || !m_imagePathEdit->isModified()) {
        QString dirName = text.toLower().replace(' ', '_');
        dirName.remove(QRegularExpression("[^a-z0-9_]"));
        m_imagePathEdit->setText(dirName);
    }
}

void AddComponentDialog::validateForm()
{
    bool valid = !m_nameEdit->text().trimmed().isEmpty() 
                 && !m_labelEdit->text().trimmed().isEmpty();
    
    // Check if type already exists
    QString typeId = generateTypeId(m_nameEdit->text().trimmed());
    if (ComponentRegistry::instance().hasComponent(typeId)) {
        m_statusLabel->setText("A component with this name already exists");
        m_statusLabel->setVisible(true);
        m_addButton->setEnabled(false);
        return;
    }
    
    m_statusLabel->setVisible(false);
    m_addButton->setEnabled(valid);
}

void AddComponentDialog::onBrowseImage()
{
    QString dir = QFileDialog::getExistingDirectory(
        this, "Select Component Image Directory",
        "assets/subsystems/",
        QFileDialog::ShowDirsOnly);
    
    if (!dir.isEmpty()) {
        QDir d(dir);
        m_imagePathEdit->setText(d.dirName());
        
        // Try to find and preview image
        QStringList imageFiles = d.entryList(QStringList() << "*.jpg" << "*.png" << "*.jpeg", QDir::Files);
        if (!imageFiles.isEmpty()) {
            QPixmap preview(d.absoluteFilePath(imageFiles.first()));
            if (!preview.isNull()) {
                m_previewLabel->setPixmap(preview.scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            }
        } else {
            m_previewLabel->setText("No images found in directory");
        }
    }
}

QString AddComponentDialog::generateTypeId(const QString& displayName) const
{
    // Convert "GPS Receiver" -> "GpsReceiver"
    QStringList words = displayName.split(' ', Qt::SkipEmptyParts);
    QString typeId;
    for (const QString& word : words) {
        if (!word.isEmpty()) {
            typeId += word.at(0).toUpper() + word.mid(1).toLower();
        }
    }
    return typeId;
}

void AddComponentDialog::onAddClicked()
{
    QString name = m_nameEdit->text().trimmed();
    QString label = m_labelEdit->text().trimmed();
    
    if (name.isEmpty() || label.isEmpty()) {
        m_statusLabel->setText("Name and label are required");
        m_statusLabel->setVisible(true);
        return;
    }
    
    // Build the component definition
    m_result.typeId = generateTypeId(name);
    m_result.displayName = name;
    m_result.label = label.toUpper();
    m_result.description = m_descriptionEdit->toPlainText().trimmed();
    m_result.imageDir = m_imagePathEdit->text().trimmed();
    m_result.iconColor = QColor("#00BCD4"); // Default teal
    
    // Parse subsystems
    QStringList subsystems;
    QString subsText = m_subsystemsEdit->toPlainText().trimmed();
    if (!subsText.isEmpty()) {
        QStringList lines = subsText.split('\n', Qt::SkipEmptyParts);
        for (const QString& line : lines) {
            QString trimmed = line.trimmed();
            if (!trimmed.isEmpty()) {
                subsystems.append(trimmed);
            }
        }
    }
    m_result.subsystems = subsystems;
    
    m_result.protocol = m_protocolCombo->currentText();
    m_result.port = m_portSpin->value();
    m_result.category = m_categoryCombo->currentText();
    m_result.shape = m_shapeCombo->currentText();
    
    // Register and save
    ComponentRegistry& registry = ComponentRegistry::instance();
    if (registry.registerComponent(m_result)) {
        registry.saveToFile();
        
        // Create image directory if it doesn't exist
        if (!m_result.imageDir.isEmpty()) {
            QString dirPath = QString("assets/subsystems/%1").arg(m_result.imageDir);
            QDir().mkpath(dirPath);
        }
        
        accept();
    } else {
        m_statusLabel->setText("Failed to register component");
        m_statusLabel->setVisible(true);
    }
}

ComponentDefinition AddComponentDialog::getComponentDefinition() const
{
    return m_result;
}
