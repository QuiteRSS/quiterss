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
    src/updateappdialog.h \
    src/feedpropertiesdialog.h \
    src/feedsview.h \
    src/updateobject.h \
    src/faviconloader.h \
    src/dbmemfilethread.h \
    src/newsfiltersdialog.h \
    src/filterrulesdialog.h \
    src/webpage.h \
    src/lineedit.h \
    src/db_func.h \
    src/webview.h

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
    src/filterrulesdialog.cpp \
    src/webpage.cpp \
    src/lineedit.cpp \
    src/db_func.cpp \
    src/webview.cpp

CONFIG(debug, debug|release) {
  BUILD_DIR = debug
} else {
  BUILD_DIR = release
  DEFINES += QT_NO_DEBUG_OUTPUT
}

DESTDIR = $${BUILD_DIR}/target/
OBJECTS_DIR = $${BUILD_DIR}/obj/
MOC_DIR = $${BUILD_DIR}/moc/
RCC_DIR = $${BUILD_DIR}/rcc/

include(3rdparty/sqlite.pri)
include(3rdparty/qtsingleapplication/qtsingleapplication.pri)
include(lang/lang.pri)

win32 {
TARGET = QuiteRSS
LIBS += libkernel32 \
        libpsapi
RC_FILE = QuiteRSSApp.rc
}

os2 {
TARGET = QuiteRSS
RC_FILE = quiterss_os2.rc
}

DISTFILES += \
    HISTORY_RU \
    HISTORY_EN \
    COPYING

unix {
  TARGET = quiterss
  CONFIG += link_pkgconfig

  isEmpty(PREFIX) {
    PREFIX =   /usr
  }

  target.path =  $$PREFIX/bin

  desktop.files = quiterss.desktop
  desktop.path =  $$PREFIX/share/applications

  icon_16.files = images/16x16/quiterss.png
  icon_24.files = images/24x24/quiterss.png
  icon_32.files = images/32x32/quiterss.png
  icon_48.files = images/48x48/quiterss.png
  icon_64.files = images/64x64/quiterss.png
  icon_16.path =  $$PREFIX/share/icons/hicolor/16x16/apps
  icon_24.path =  $$PREFIX/share/icons/hicolor/24x24/apps
  icon_32.path =  $$PREFIX/share/icons/hicolor/32x32/apps
  icon_48.path =  $$PREFIX/share/icons/hicolor/48x48/apps
  icon_64.path =  $$PREFIX/share/icons/hicolor/64x64/apps

  translations.files = $$DESTDIR/lang/*.qm
  translations.path =  $$PREFIX/share/quiterss/lang
  translations.CONFIG += no_check_exist

  sound.files = sound/*.wav
  sound.path = $$PREFIX/share/quiterss/sound

  INSTALLS += target desktop icon_16 icon_24 icon_32 icon_48 icon_64 \
              translations sound
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

OTHER_FILES += \
    HISTORY_RU \
    HISTORY_EN
