#-------------------------------------------------
#
# Project created by QtCreator 2015-09-01T16:46:01
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets multimedia

TARGET = FIR
TEMPLATE = app


SOURCES += main.cpp\
        fir.cpp \
    func.cpp

HEADERS  += fir.h \
    func.h

FORMS    += fir.ui

RESOURCES += \
    resource.qrc

DISTFILES += \
    icon.rc

RC_FILE = icon.rc
