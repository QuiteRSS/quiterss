QT += core gui network xml webkit sql

TARGET = QtRss
TEMPLATE = app

HEADERS += rsslisting.h \
    VersionNo.h \
    optionsdialog.h \
    qtsingleapplication/qtsingleapplication.h \
    qtsingleapplication/qtlockedfile.h \
    qtsingleapplication/qtlocalpeer.h \
    addfeeddialog.h
SOURCES += main.cpp rsslisting.cpp \
    optionsdialog.cpp \
    qtsingleapplication/qtsingleapplication.cpp \
    qtsingleapplication/qtlockedfile_win.cpp \
    qtsingleapplication/qtlockedfile_unix.cpp \
    qtsingleapplication/qtlockedfile.cpp \
    qtsingleapplication/qtlocalpeer.cpp \
    addfeeddialog.cpp

RESOURCES += \
    qtrss.qrc
RC_FILE = QtRssApp.rc

BUILD_DIR = release
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
  BUILD_DIR = debug
}

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8
include(lang/lang.pri)






