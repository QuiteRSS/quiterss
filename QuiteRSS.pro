QT += core gui network xml webkit sql

TEMPLATE = app

INCLUDEPATH += . \
               3rdparty/qtsingleapplication

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
    src/updateappdialog.h \
    src/feedpropertiesdialog.h \
    src/feedsview.h \
    3rdparty/qtsingleapplication/qtsingleapplication.h \
    3rdparty/qtsingleapplication/qtlockedfile.h \
    3rdparty/qtsingleapplication/qtlocalpeer.h
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
    src/updateappdialog.cpp \
    src/feedpropertiesdialog.cpp \
    src/feedsview.cpp \
    3rdparty/qtsingleapplication/qtsingleapplication.cpp \
    3rdparty/qtsingleapplication/qtlockedfile_win.cpp \
    3rdparty/qtsingleapplication/qtlockedfile_unix.cpp \
    3rdparty/qtsingleapplication/qtlockedfile.cpp \
    3rdparty/qtsingleapplication/qtlocalpeer.cpp

BUILD_DIR = release
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
  BUILD_DIR = debug
}

win32 {
TARGET = QuiteRSS
LIBS += libkernel32 \
        libpsapi
RC_FILE = QuiteRSSApp.rc
INCLUDEPATH += ./3rdparty/sqlite
HEADERS += 3rdparty/sqlite/sqlite3.h
SOURCES += 3rdparty/sqlite/sqlite3.c
include(lang/lang.pri)
}

os2 {
TARGET = QuiteRSS
RC_FILE = quiterss_os2.rc
INCLUDEPATH += ./3rdparty/sqlite
HEADERS += 3rdparty/sqlite/sqlite3.h
SOURCES += 3rdparty/sqlite/sqlite3.c
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
  icon_48.files = images/48x48/quiterss.png
  icon_64.files = images/64x64/quiterss.png
  translations.files = lang/*.qm
  isEmpty(PREFIX) {
    PREFIX =   /usr
  }
  target.path =  $$PREFIX/bin
  desktop.path =  $$PREFIX/share/applications
  icon_16.path =  $$PREFIX/share/icons/hicolor/16x16/apps
  icon_24.path =  $$PREFIX/share/icons/hicolor/24x24/apps
  icon_32.path =  $$PREFIX/share/icons/hicolor/32x32/apps
  icon_48.path =  $$PREFIX/share/icons/hicolor/48x48/apps
  icon_64.path =  $$PREFIX/share/icons/hicolor/64x64/apps
  translations.path =  $$PREFIX/share/quiterss/lang
  INSTALLS += target desktop icon_16 icon_24 icon_32 icon_48 icon_64 translations
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8
