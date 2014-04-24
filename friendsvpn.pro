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
    datareceiver.cpp \
    rawsocket.cpp \
    controlplane/connectioninitiator.cpp \
    controlplane/controlplaneclient.cpp \
    controlplane/controlplaneserver.cpp \
    user.cpp \
    controlplane/threadedcpserver.cpp \
    controlplane/sslserverthread.cpp \
    controlplane/controlplaneconnection.cpp \
    controlplane/sslsocket.cpp \
    proxy.cpp \
    bonjour/bonjourregistrar.cpp \
    dataplane/dataplaneconnection.cpp \
    dataplane/dataplaneclient.cpp \
    ph2phtp_parser.cpp \
    privilegeescalator.cpp

HEADERS  += \
    graphic/systray.h \
    bonjour/bonjourbrowser.h \
    bonjour/bonjourrecord.h \
    bonjour/bonjourdiscoverer.h \
    bonjour/bonjourresolver.h \
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
    controlplane/sslserverthread.h \
    controlplane/controlplaneconnection.h \
    controlplane/sslsocket.h \
    proxy.h \
    bonjour/bonjourregistrar.h \
    dataplane/dataplaneconnection.h \
    dataplane/dataplaneclient.h \
    dataplane/dataplaneconfig.h \
    ph2phtp_parser.h \
    privilegeescalator.h

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
