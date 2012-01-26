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
    src/feedpropertiesdialog.h \
    src/feedsview.h
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
    src/feedsview.cpp

BUILD_DIR = release
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
  BUILD_DIR = debug
}

win32 {
TARGET = QuiteRSS
LIBS += libkernel32 \
        libpsapi
RC_FILE = QuiteRSSApp.rc
HEADERS += src/sqlite/sqlite3.h
SOURCES += src/sqlite/sqlite3.c
include(lang/lang.pri)
}

os2 {
TARGET = QuiteRSS
RC_FILE = quiterss_os2.rc
HEADERS += src/sqlite/sqlite3.h
SOURCES += src/sqlite/sqlite3.c
include(lang/lang.pri)
}

unix {
  TARGET = quiterss
  CONFIG += link_pkgconfig
  PKGCONFIG += sqlite3
  TRANSLATIONS += lang/quiterss_en.ts lang/quiterss_de.ts lang/quiterss_ru.ts
  desktop.files = quiterss.desktop
  icon_16.files = images/16x16/quiterss.png
  icon_24.files = images/24x24/quiterss.png
  icon_32.files = images/32x32/quiterss.png
  translations.files = lang/*.qm
  isEmpty(PREFIX) {
    PREFIX =   /usr
  }
  target.path =  $$PREFIX/bin
  desktop.path =  $$PREFIX/share/applications
  icon_16.path =  $$PREFIX/share/icons/hicolor/16x16
  icon_24.path =  $$PREFIX/share/icons/hicolor/24x24
  icon_32.path =  $$PREFIX/share/icons/hicolor/32x32
  translations.path =  $$PREFIX/share/quiterss/lang
  INSTALLS += target desktop icon_16 icon_24 icon_32 translations
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8
