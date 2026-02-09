QT += core gui widgets network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RuntimeApp
TEMPLATE = app

CONFIG += c++11

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    component.cpp \
    canvas.cpp \
    analytics.cpp \
    messageserver.cpp \
    voicealertmanager.cpp

HEADERS += \
    mainwindow.h \
    component.h \
    canvas.h \
    analytics.h \
    messageserver.h \
    voicealertmanager.h

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
