QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11
CONFIG += static

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

# linux magic
DEFINES += "_XOPEN_SOURCE=500"
DEFINES += _DEFAULT_SOURCE

# link giflib and libwebp
QMAKE_LFLAGS += " -lgif -lm -lwebp "
DEFINES += WEBP_HAVE_GIF

#win32 { QMAKE_LFLAGS += " -lpcre2-posix -lpcre2-8 -municode " }
QMAKE_LFLAGS_WINDOWS += " -lpcre2-posix -lpcre2-8 -municode " 

# my qt gui
SOURCES += \
    main.cpp \
    mainwindow.cpp

# ezspritesheet core
SOURCES += \
    ../../animation.c \
    ../../common.c \
    ../../exporter.c \
    ../../ezspritesheet.c \
    ../../file.c \
    ../../nftw_utf8.c \
    ../../rectangle.c

# ezspritesheet exporters
SOURCES += \
    ../../exporter/c99.c \
    ../../exporter/xml.c \
    ../../exporter/json.c

# ezspritesheet dependency: RectangleBinPack
SOURCES += \
    ../../../dep/RectangleBinPack/GuillotineBinPack.cpp \
    ../../../dep/RectangleBinPack/MaxRectsBinPack.cpp \
    ../../../dep/RectangleBinPack/SkylineBinPack.cpp \
    ../../../dep/RectangleBinPack/Rect.cpp

# ezspritesheet dependency: misc libwebp components
SOURCES += \
    ../../../dep/libwebp/examples/anim_util.c \
    ../../../dep/libwebp/examples/gifdec.c \
    ../../../dep/libwebp/src/demux/demux.c \
    ../../../dep/libwebp/src/demux/anim_decode.c \
    ../../../dep/libwebp/imageio/imageio_util.c

# misc headers
INCLUDEPATH += \
    ../../../dep \
    ../../../dep/libwebp/src \
    ../../../dep/libwebp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

TRANSLATIONS += \
    ezspritesheet-qt_en_US.ts

CONFIG(debug, debug|release) {
    DESTDIR = ../../../bin/qt/debug
} else {
    DESTDIR = ../../../bin/qt/release
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
