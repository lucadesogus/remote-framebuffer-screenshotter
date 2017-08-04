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

unix:CONFIG(debug, debug|release) DESTDIR = $$PWD/build/linux/debug
unix:CONFIG(release, debug|release) DESTDIR = $$PWD/build/linux/release

DEFINES += QT_DEPRECATED_WARNINGS QT_WIDGETS_LIB QT_PRINTSUPPORT_LIB

INCLUDEPATH += . \
               ./inc \
               ./lib/inc #change this line with the path in which header files of libssh resides in your env

win32: INCLUDEPATH += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Include) #may differs in your env
else:unix: INCLUDEPATH += /usr/lib/x86_64-linux-gnu #may differs in your env

SOURCES += \
        ./src/rfbss.cpp \
        ./src/clickable_label.cpp \
        ./src/main.cpp

HEADERS += \
        ./inc/rfbss.h \
        ./inc/clickable_label.h

FORMS += \
        ./ui/rfbss.ui

#libssh.lib/.a link, folder path may differs
win32:CONFIG(release, debug|release) LIBS += -L"lib/bin/win" -lssh
else:win32:CONFIG(debug, debug|release) LIBS += -L"lib/bin/win" -lssh
else:unix:CONFIG(release, debug|release) LIBS += -L"/usr/local/lib" -lssh
else:unix:CONFIG(debug, debug|release) LIBS += -L"/usr/local/lib" -lssh

#win32:CONFIG(release, debug|release) LIBS += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/WS2_32.lib)
#else:win32:CONFIG(debug, debug|release) LIBS += $$quote(C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Lib/WS2_32.lib)


