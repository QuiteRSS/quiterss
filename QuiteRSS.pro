QT += core gui network xml webkit sql

TEMPLATE = app

HEADERS += \
    src/VersionNo.h \
    src/updatethread.h \
    src/rsslisting.h \
    src/parsethread.h \
    src/parseobject.h \
    src/optionsdialog.h \
    src/newsview.h \
    src/newsmodel.h \
    src/newsheader.h \
    src/feedsmodel.h \
    src/delegatewithoutfocus.h \
    src/addfeeddialog.h \
    src/aboutdialog.h \
    src/qtsingleapplication/qtsingleapplication.h \
    src/qtsingleapplication/qtlockedfile.h \
    src/qtsingleapplication/qtlocalpeer.h \
    src/updateappdialog.h \
    src/feedpropertiesdialog.h
SOURCES += \
    src/updatethread.cpp \
    src/rsslisting.cpp \
    src/parsethread.cpp \
    src/parseobject.cpp \
    src/optionsdialog.cpp \
    src/newsview.cpp \
    src/newsmodel.cpp \
    src/newsheader.cpp \
    src/main.cpp \
    src/feedsmodel.cpp \
    src/delegatewithoutfocus.cpp \
    src/addfeeddialog.cpp \
    src/aboutdialog.cpp \
    src/qtsingleapplication/qtsingleapplication.cpp \
    src/qtsingleapplication/qtlockedfile_win.cpp \
    src/qtsingleapplication/qtlockedfile_unix.cpp \
    src/qtsingleapplication/qtlockedfile.cpp \
    src/qtsingleapplication/qtlocalpeer.cpp \
    src/updateappdialog.cpp \
    src/feedpropertiesdialog.cpp \

win32 {
TARGET = QuiteRSS

LIBS += libkernel32 \
        libpsapi

RC_FILE = QuiteRSSApp.rc
}

win32 || os2 {
HEADERS += \
    src/sqlite/sqlite3.h
SOURCES += \
    src/sqlite/sqlite3.c
}

uni {
TARGET = quiterss
}

unix {
  CONFIG += link_pkgconfig
  PKGCONFIG += sqlite3
  translations.files = lang/*.qm
  desktop.files = quiterss.desktop
  icon.files = quiterss.ico
  isEmpty(PREFIX) {
    PREFIX =   /usr
  }
  target.path =  $$PREFIX/bin
  translations.path =  $$PREFIX/share/quiterss/lang
  desktop.path =  $$PREFIX/share/applications
  icon.path =  $$PREFIX/share/pixmaps
  INSTALLS +=  target translations  desktop  icon
}

os2 {
  quiterss_os2.rc
}

RESOURCES += \
    QuiteRSS.qrc

BUILD_DIR = release
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
  BUILD_DIR = debug
}

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8
include(lang/lang.pri)

