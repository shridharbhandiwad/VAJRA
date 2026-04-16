QT += core gui widgets network charts printsupport qml quick quickwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# Optional Qt SerialPort module (RS422/RS232 support)
# Comment this out if the module is not installed.
qtHaveModule(serialport) {
    QT += serialport
    DEFINES += QT_SERIALPORT_LIB
    message("Qt SerialPort module found – RS422/RS232 enabled")
} else {
    message("Qt SerialPort module NOT found – RS422/RS232 will run in simulation mode")
}

# Optional Qt SerialBus module (Modbus / CAN support)
# Comment this out if the module is not installed.
qtHaveModule(serialbus) {
    QT += serialbus
    DEFINES += QT_SERIALBUS_LIB
    message("Qt SerialBus module found – Modbus/CAN enabled")
} else {
    message("Qt SerialBus module NOT found – Modbus/CAN will run in simulation mode")
}

TARGET = UnifiedApp
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    logindialog.cpp \
    mainwindow.cpp \
    component.cpp \
    subcomponent.cpp \
    designsubcomponent.cpp \
    connection.cpp \
    canvas.cpp \
    componentlist.cpp \
    analytics.cpp \
    analyticsdashboard.cpp \
    messageserver.cpp \
    componentregistry.cpp \
    addcomponentdialog.cpp \
    editcomponentdialog.cpp \
    voicealertmanager.cpp \
    enlargedcomponentview.cpp \
    thememanager.cpp \
    protocolhandler.cpp \
    tcpprotocolhandler.cpp \
    udpprotocolhandler.cpp \
    serialprotocolhandler.cpp \
    modbusprotocolhandler.cpp \
    canprotocolhandler.cpp \
    protocolhandlerfactory.cpp \
    trmgridview.cpp \
    RadarModel.cpp \
    UdpReceiver.cpp \
    SimulatorEngine.cpp \
    CommandSender.cpp \
    radarantennawidget.cpp

HEADERS += \
    userrole.h \
    logindialog.h \
    mainwindow.h \
    component.h \
    subcomponent.h \
    designsubcomponent.h \
    connection.h \
    canvas.h \
    componentlist.h \
    analytics.h \
    analyticsdashboard.h \
    messageserver.h \
    componentregistry.h \
    addcomponentdialog.h \
    editcomponentdialog.h \
    voicealertmanager.h \
    enlargedcomponentview.h \
    thememanager.h \
    protocolhandler.h \
    tcpprotocolhandler.h \
    udpprotocolhandler.h \
    serialprotocolhandler.h \
    modbusprotocolhandler.h \
    canprotocolhandler.h \
    protocolhandlerfactory.h \
    trmgridview.h \
    AntennaConfig.h \
    RadarModel.h \
    UdpReceiver.h \
    SimulatorEngine.h \
    CommandSender.h \
    radarantennawidget.h

RESOURCES += \
    resources.qrc

# Copy components.json to build directory
config_file.files = components.json
config_file.path = $$OUT_PWD
INSTALLS += config_file

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
