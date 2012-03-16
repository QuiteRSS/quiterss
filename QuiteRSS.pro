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
    src/updateobject.h \
    src/faviconloader.h \
    src/dbmemfilethread.h \
    src/newsfiltersdialog.h \
    src/filterrulesdialog.h
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
    src/updateobject.cpp \
    src/faviconloader.cpp \
    src/dbmemfilethread.cpp \
    src/newsfiltersdialog.cpp \
    src/filterrulesdialog.cpp

BUILD_DIR = release
if(!debug_and_release|build_pass):CONFIG(debug, debug|release) {
  BUILD_DIR = debug
}

include(3rdparty/sqlite.pri)
include(3rdparty/qtsingleapplication/qtsingleapplication.pri)

win32 {
TARGET = QuiteRSS
LIBS += libkernel32 \
        libpsapi
RC_FILE = QuiteRSSApp.rc
include(lang/lang.pri)
}

os2 {
TARGET = QuiteRSS
RC_FILE = quiterss_os2.rc
include(lang/lang.pri)
}

unix {
  TARGET = quiterss
  CONFIG += link_pkgconfig
  TRANSLATIONS += lang/quiterss_en.ts lang/quiterss_de.ts \
                  lang/quiterss_ru.ts lang/quiterss_es.ts
  desktop.files = quiterss.desktop
  icon_16.files = images/16x16/quiterss.png
  icon_24.files = images/24x24/quiterss.png
  icon_32.files = images/32x32/quiterss.png
  icon_48.files = images/48x48/quiterss.png
  icon_64.files = images/64x64/quiterss.png
  translations.files = lang/*.qm
  sound.files = sound/*.wav
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
  sound.path = $$PREFIX/share/quiterss/sound
  INSTALLS += target desktop icon_16 icon_24 icon_32 icon_48 icon_64 \
              translations sound
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

OTHER_FILES += \
    history_en \
    history_ru
