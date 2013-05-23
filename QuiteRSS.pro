# VCS revision info
REVFILE = src/VersionRev.h
QMAKE_DISTCLEAN += $$REVFILE
# tortoisehg-2.8 running in win32 hangs with 3 \ in sources, but works fine in linux
THG_WIN32_FIXME = '\\'
exists(.hg) {
  VERSION_REV = $$system(hg parents --template '{rev}')
  count(VERSION_REV, 1) {
    os2|win32 {
      # FIXME
      VERSION_REV = hg-$$VERSION_REV
    } else {
      VERSION_REV = hg-$$VERSION_REV-$$system(hg parents --template '{node\\|short}')
    }
  } else {
    VERSION_REV = 0
  }
  message(VCS revision: $$VERSION_REV)

  os2|win32 {
    system(echo $${LITERAL_HASH}define VCS_REVISION \"$$VERSION_REV\" > $$REVFILE)
  } else {
    system(echo \\$${LITERAL_HASH}define VCS_REVISION $$THG_WIN32_FIXME\"$$VERSION_REV$$THG_WIN32_FIXME\" > $$REVFILE)
  }
} else:exists(.git) {
  VERSION_REV = $$system(git rev-list --count HEAD)
  count(VERSION_REV, 1) {
  VERSION_REV = git-$$VERSION_REV-$$system(git rev-parse --short HEAD)
  } else {
    VERSION_REV = 0
  }
  message(VCS revision: $$VERSION_REV)

  os2|win32 {
    system(echo $${LITERAL_HASH}define VCS_REVISION \"$$VERSION_REV\" > $$REVFILE)
  } else {
    system(echo \\$${LITERAL_HASH}define VCS_REVISION $$THG_WIN32_FIXME\"$$VERSION_REV$$THG_WIN32_FIXME\" > $$REVFILE)
  }
} else:!exists($$REVFILE) {
  VERSION_REV = 0
  message(VCS revision: $$VERSION_REV)

  os2|win32 {
    system(echo $${LITERAL_HASH}define VCS_REVISION \"$$VERSION_REV\" > $$REVFILE)
  } else {
    system(echo \\$${LITERAL_HASH}define VCS_REVISION $$THG_WIN32_FIXME\"$$VERSION_REV$$THG_WIN32_FIXME\" > $$REVFILE)
  }
}

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
    src/delegatewithoutfocus.h \
    src/aboutdialog.h \
    src/updateappdialog.h \
    src/feedpropertiesdialog.h \
    src/updateobject.h \
    src/dbmemfilethread.h \
    src/newsfiltersdialog.h \
    src/filterrulesdialog.h \
    src/webpage.h \
    src/lineedit.h \
    src/db_func.h \
    src/webview.h \
    src/addfeedwizard.h \
    src/newstabwidget.h \
    src/findtext.h \
    src/notifications.h \
    src/findfeed.h \
    src/googlereader.h \
    src/feedstreeview.h \
    src/feedstreemodel.h \
    src/VersionRev.h \
    src/splashscreen.h \
    src/addfolderdialog.h \
    src/labeldialog.h \
    src/dialog.h \
    src/updatedelayer.h \
    src/authenticationdialog.h \
    src/networkmanager.h \
    src/cookiejar.h \
    src/faviconthread.h \
    src/faviconobject.h \
    src/customizetoolbardialog.h \
    src/plugins/webpluginfactory.h \
    src/plugins/clicktoflash.h \
    src/downloads/downloadmanager.h \
    src/downloads/downloaditem.h \
    src/tabbar.h

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
    src/delegatewithoutfocus.cpp \
    src/aboutdialog.cpp \
    src/updateappdialog.cpp \
    src/feedpropertiesdialog.cpp \
    src/updateobject.cpp \
    src/dbmemfilethread.cpp \
    src/newsfiltersdialog.cpp \
    src/filterrulesdialog.cpp \
    src/webpage.cpp \
    src/lineedit.cpp \
    src/db_func.cpp \
    src/webview.cpp \
    src/addfeedwizard.cpp \
    src/newstabwidget.cpp \
    src/findtext.cpp \
    src/notifications.cpp \
    src/findfeed.cpp \
    src/googlereader.cpp \
    src/feedstreeview.cpp \
    src/feedstreemodel.cpp \
    src/splashscreen.cpp \
    src/addfolderdialog.cpp \
    src/labeldialog.cpp \
    src/dialog.cpp \
    src/updatedelayer.cpp \
    src/authenticationdialog.cpp \
    src/networkmanager.cpp \
    src/cookiejar.cpp \
    src/faviconthread.cpp \
    src/faviconobject.cpp \
    src/customizetoolbardialog.cpp \
    src/plugins/webpluginfactory.cpp \
    src/plugins/clicktoflash.cpp \
    src/downloads/downloadmanager.cpp \
    src/downloads/downloaditem.cpp \
    src/tabbar.cpp

INCLUDEPATH +=  $$PWD/src/downloads \
                $$PWD/src/plugins \
                $$PWD/src \

CONFIG(debug, debug|release) {
  BUILD_DIR = $$OUT_PWD/debug
} else {
  BUILD_DIR = $$OUT_PWD/release
  DEFINES += QT_NO_DEBUG_OUTPUT
}

DESTDIR = $${BUILD_DIR}/target
OBJECTS_DIR = $${BUILD_DIR}/obj
MOC_DIR = $${BUILD_DIR}/moc
RCC_DIR = $${BUILD_DIR}/rcc

include(3rdparty/qtsingleapplication/qtsingleapplication.pri)
include(3rdparty/qyursqltreeview/qyursqltreeview.pri)
include(lang/lang.pri)

os2|win32 {
  TARGET = QuiteRSS

  include(3rdparty/sqlite.pri)
}

win32 {
  RC_FILE = QuiteRSSApp.rc
}

win32-g++ {
  LIBS += libkernel32 \
          libpsapi
}

win32-msvc* {
  LIBS += -lpsapi
  LIBS += -lShell32

  QMAKE_CXXFLAGS += -D__PRETTY_FUNCTION__=__FUNCTION__
  QMAKE_CFLAGS += -D__PRETTY_FUNCTION__=__FUNCTION__
}

os2 {
  RC_FILE = quiterss_os2.rc
}

DISTFILES += \
    HISTORY_RU \
    HISTORY_EN \
    README \
    COPYING \
    AUTHORS \
    CHANGELOG

unix {
  TARGET = quiterss
  CONFIG += link_pkgconfig
  LIBS += -lsqlite3

  isEmpty(PREFIX) {
    PREFIX =   /usr
  }

  target.path =  $$PREFIX/bin

  desktop.files = quiterss.desktop
  desktop.path =  $$PREFIX/share/applications

  target1.files = images/48x48/quiterss.png
  target1.path =  $$PREFIX/share/pixmaps

  icon_16.files =  images/16x16/quiterss.png
  icon_32.files =  images/32x32/quiterss.png
  icon_48.files =  images/48x48/quiterss.png
  icon_64.files =  images/64x64/quiterss.png
  icon_128.files = images/128x128/quiterss.png
  icon_256.files = images/256x256/quiterss.png
  icon_16.path =  $$PREFIX/share/icons/hicolor/16x16/apps
  icon_32.path =  $$PREFIX/share/icons/hicolor/32x32/apps
  icon_48.path =  $$PREFIX/share/icons/hicolor/48x48/apps
  icon_64.path =  $$PREFIX/share/icons/hicolor/64x64/apps
  icon_128.path = $$PREFIX/share/icons/hicolor/128x128/apps
  icon_256.path = $$PREFIX/share/icons/hicolor/256x256/apps

  translations.files = $$DESTDIR/lang/*.qm
  translations.path =  $$PREFIX/share/quiterss/lang
  translations.CONFIG += no_check_exist

  sound.files = sound/*.wav
  sound.path = $$PREFIX/share/quiterss/sound

  style.files = style/*.*
  style.path = $$PREFIX/share/quiterss/style

  INSTALLS += target desktop target1
  INSTALLS += icon_16 icon_32 icon_48 icon_64 icon_128 icon_256 \
              translations sound style
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

OTHER_FILES += \
    HISTORY_RU \
    HISTORY_EN \
    README \
    COPYING \
    AUTHORS \
    CHANGELOG
