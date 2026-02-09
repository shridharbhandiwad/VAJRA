QT += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UnifiedApp
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    logindialog.cpp \
    mainwindow.cpp \
    component.cpp \
    subcomponent.cpp \
    connection.cpp \
    canvas.cpp \
    componentlist.cpp \
    analytics.cpp \
    messageserver.cpp \
    componentregistry.cpp \
    addcomponentdialog.cpp \
    voicealertmanager.cpp

HEADERS += \
    logindialog.h \
    mainwindow.h \
    component.h \
    subcomponent.h \
    connection.h \
    canvas.h \
    componentlist.h \
    analytics.h \
    messageserver.h \
    componentregistry.h \
    addcomponentdialog.h \
    voicealertmanager.h

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
