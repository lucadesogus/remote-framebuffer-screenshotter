#-------------------------------------------------
#
# Project created by Luca Desogus 2017-08-01T15:44:37
#
#-------------------------------------------------

QT       += core gui printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RFBSS
TEMPLATE = app

win32:CONFIG(debug, debug|release) DESTDIR = $$PWD/build/win/debug
win32:CONFIG(release, debug|release) DESTDIR = $$PWD/build/win/release

DEFINES += QT_DEPRECATED_WARNINGS WIN64 QT_DLL QT_WIDGETS_LIB QT_PRINTSUPPORT_LIB

INCLUDEPATH += . \
               ./inc \
               ./lib/inc

win32:CONFIG(release, debug|release) LIBS += -L$$PWD/lib/bin/win/ -lssh -lssleay32 -llibeay32
win32:CONFIG(release, debug|release) LIBS += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/WS2_32.lib)

win32:CONFIG(debug, debug|release) LIBS += -L$$PWD/lib/bin/win/ -lssh -lssleay32 -llibeay32
win32:CONFIG(debug, debug|release) LIBS += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/WS2_32.lib)

win32: INCLUDEPATH += $$PWD/lib/bin/win
win32: DEPENDPATH += $$PWD/lib/bin/win


win32: INCLUDEPATH += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Include)
win32: DEPENDPATH += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Include)
win32: DEPENDPATH += ./lib/inc/libssh


SOURCES += \
        ./src/rfbss.cpp \
        ./src/clickable_label.cpp \
        ./src/main.cpp

HEADERS += \
        ./inc/rfbss.h \
        ./inc/clickable_label.h

FORMS += \
        ./ui/rfbss.ui





