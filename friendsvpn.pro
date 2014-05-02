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
    bonjour/bonjourbrowser.cpp \
    bonjour/bonjourdiscoverer.cpp \
    bonjour/bonjourresolver.cpp \
    bonjoursql.cpp \
    poller.cpp \
    connectioninitiator.cpp \
    controlplane/controlplaneclient.cpp \
    controlplane/controlplaneserver.cpp \
    user.cpp \
    controlplane/controlplaneconnection.cpp \
    controlplane/sslsocket.cpp \
    proxy.cpp \
    bonjour/bonjourregistrar.cpp \
    dataplane/dataplaneclient.cpp \
    ph2phtp_parser.cpp \
    dataplane/dataplaneserver.cpp \
    dataplane/dataplaneconnection.cpp \
    abstractplaneconnection.cpp

HEADERS  += \
    graphic/systray.h \
    bonjour/bonjourbrowser.h \
    bonjour/bonjourrecord.h \
    bonjour/bonjourdiscoverer.h \
    bonjour/bonjourresolver.h \
    bonjoursql.h \
    config.h \
    poller.h \
    connectioninitiator.h \
    controlplane/controlplaneserver.h \
    controlplane/controlplaneclient.h \
    user.h \
    controlplane/controlplaneconnection.h \
    controlplane/sslsocket.h \
    proxy.h \
    bonjour/bonjourregistrar.h \
    dataplane/dataplaneclient.h \
    dataplane/dataplaneconfig.h \
    ph2phtp_parser.h \
    dataplane/dataplaneserver.h \
    dataplane/dataplaneconnection.h \
    abstractplaneconnection.h

FORMS    +=

INCLUDEPATH += $$_PRO_FILE_PWD_/libmaia
LIBS += $$_PRO_FILE_PWD_/libmaia/libmaia.a

# if on linux
unix:!macx {
    INCLUDEPATH += /usr/lib
    LIBS += /usr/lib/libdns_sd.so
}

INCLUDEPATH += $$_PRO_FILE_PWD_/openssl-1.0.1g
LIBS += $$_PRO_FILE_PWD_/openssl-1.0.1g/libssl.a
LIBS += $$_PRO_FILE_PWD_/openssl-1.0.1g/libcrypto.a

RESOURCES += \
    images.qrc
