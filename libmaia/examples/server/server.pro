######################################################################
# Automatically generated by qmake (2.01a) Fr Mai 25 19:04:58 2007
######################################################################

TEMPLATE = app
INCLUDEPATH += . ../../
LIBS += ../../libmaia.a

TARGET = server
DEPENDPATH += .
INCLUDEPATH += .
QT += xml network
QT -= gui
CONFIG += qt silent debug

# Input
HEADERS += server.h
SOURCES += server.cpp server_main.cpp