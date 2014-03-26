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
    pollercontroller.cpp \
    datareceiver.cpp

HEADERS  += \
    graphic/systray.h \
    bonjourbrowser.h \
    bonjourrecord.h \
    bonjourdiscoverer.h \
    bonjourresolver.h \
    bonjoursql.h \
    config.h \
    poller.h \
    pollercontroller.h \
    datareceiver.h

FORMS    +=

INCLUDEPATH += $$_PRO_FILE_PWD_/libmaia
LIBS += $$_PRO_FILE_PWD_/libmaia/libmaia.a

RESOURCES += \
    images.qrc
