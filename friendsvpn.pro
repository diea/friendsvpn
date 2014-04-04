#-------------------------------------------------
#
# Project created by QtCreator 2014-02-21T11:04:45
#
#-------------------------------------------------

QT       += core gui network sql xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = friendsvpn
TEMPLATE = app


SOURCES += main.cpp \
    graphic/systray.cpp \
    bonjourbrowser.cpp \
    bonjourdiscoverer.cpp \
    bonjourresolver.cpp \
    bonjoursql.cpp \
    poller.cpp \
    datareceiver.cpp \
    rawsocket.cpp \
    controlplane/connectioninitiator.cpp \
    controlplane/controlplaneclient.cpp \
    controlplane/controlplaneserver.cpp \
    user.cpp \
    controlplane/threadedcpserver.cpp \
    controlplane/sslserverthread.cpp

HEADERS  += \
    graphic/systray.h \
    bonjourbrowser.h \
    bonjourrecord.h \
    bonjourdiscoverer.h \
    bonjourresolver.h \
    bonjoursql.h \
    config.h \
    poller.h \
    datareceiver.h \
    rawsocket.h \
    controlplane/connectioninitiator.h \
    controlplane/controlplaneserver.h \
    controlplane/controlplaneclient.h \
    user.h \
    controlplane/threadedcpserver.h \
    controlplane/sslserverthread.h

FORMS    +=

INCLUDEPATH += $$_PRO_FILE_PWD_/libmaia
LIBS += $$_PRO_FILE_PWD_/libmaia/libmaia.a

# if on linux
unix:!macx {
    INCLUDEPATH += /usr/lib
    LIBS += /usr/lib/libdns_sd.so
}

RESOURCES += \
    images.qrc
