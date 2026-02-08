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
    canvas.cpp \
    componentlist.cpp \
    analytics.cpp \
    messageserver.cpp

HEADERS += \
    logindialog.h \
    mainwindow.h \
    component.h \
    canvas.h \
    componentlist.h \
    analytics.h \
    messageserver.h

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
