# VCS revision info
REVFILE = src/VersionRev.h
QMAKE_DISTCLEAN += $$REVFILE
exists(.git) {
  VERSION_REV = $$system(git rev-list master --count)
  count(VERSION_REV, 1) {
    os2|win32|mac {
      # FIXME
      VERSION_REV = $$VERSION_REV
    } else {
      VERSION_REV = git-$$VERSION_REV-$$system(git rev-parse --short HEAD)
    }
  } else {
    VERSION_REV = 0
  }
  !build_pass:message(VCS revision: $$VERSION_REV)

  os2|win32 {
    system(echo $${LITERAL_HASH}define VCS_REVISION $$VERSION_REV > $$REVFILE)
  } else {
    system(echo \\$${LITERAL_HASH}define VCS_REVISION \\\"$$VERSION_REV\\\" > $$REVFILE)
  }
} else:!exists($$REVFILE) {
  VERSION_REV = 0
  !build_pass:message(VCS revision: $$VERSION_REV)

  os2|win32 {
    system(echo $${LITERAL_HASH}define VCS_REVISION $$VERSION_REV > $$REVFILE)
  } else {
    system(echo \\$${LITERAL_HASH}define VCS_REVISION \\\"$$VERSION_REV\\\" > $$REVFILE)
  }
}

isEqual(QT_MAJOR_VERSION, 5) {
  QT += widgets webkitwidgets network xml printsupport sql multimedia
  DEFINES += HAVE_QT5
} else {
  QT += core gui network xml webkit sql
  os2 {
    DISABLE_PHONON = 1
  }
  isEmpty(DISABLE_PHONON) {
    QT += phonon
    DEFINES += HAVE_PHONON
  }
}

unix:!mac:DEFINES += HAVE_X11

TEMPLATE = app

HEADERS += \
    src/VersionNo.h \
    src/parseobject.h \
    src/optionsdialog.h \
    src/newsview/newsview.h \
    src/newsview/newsmodel.h \
    src/newsview/newsheader.h \
    src/aboutdialog.h \
    src/updateappdialog.h \
    src/feedpropertiesdialog.h \
    src/addfeedwizard.h \
    src/newstabwidget.h \
    src/findtext.h \
    src/findfeed.h \
    src/feedsview/feedsview.h \
    src/feedsview/feedsmodel.h \
    src/VersionRev.h \
    src/addfolderdialog.h \
    src/labeldialog.h \
    src/faviconobject.h \
    src/customizetoolbardialog.h \
    src/plugins/webpluginfactory.h \
    src/plugins/clicktoflash.h \
    src/downloads/downloadmanager.h \
    src/downloads/downloaditem.h \
    src/tabbar.h \
    src/categoriestreewidget.h \
    src/cleanupwizard.h \
    src/updatefeeds.h \
    src/requestfeed.h \
    src/notifications/notificationsfeeditem.h \
    src/notifications/notificationsnewsitem.h \
    src/notifications/notificationswidget.h \
    src/application/mainapplication.h \
    src/application/settings.h \
    src/application/logfile.h \
    src/application/mainwindow.h \
    src/adblock/adblocktreewidget.h \
    src/adblock/adblocksubscription.h \
    src/adblock/adblocksearchtree.h \
    src/adblock/adblockrule.h \
    src/adblock/adblockmanager.h \
    src/adblock/adblockicon.h \
    src/adblock/adblockdialog.h \
    src/adblock/adblockblockednetworkreply.h \
    src/adblock/adblockaddsubscriptiondialog.h \
    src/adblock/followredirectreply.h \
    src/application/splashscreen.h \
    src/network/authenticationdialog.h \
    src/network/cookiejar.h \
    src/network/networkmanager.h \
    src/webview/locationbar.h \
    src/webview/rssdetectionwidget.h \
    src/webview/webpage.h \
    src/webview/webview.h \
    src/database/database.h \
    src/common/common.h \
    src/common/delegatewithoutfocus.h \
    src/common/dialog.h \
    src/common/lineedit.h \
    src/common/toolbutton.h \
    src/newsfilters/filterrulesdialog.h \
    src/newsfilters/newsfiltersdialog.h \
    src/newsfilters/itemcondition.h \
    src/newsfilters/itemaction.h \
    src/network/sslerrordialog.h \
    src/network/networkmanagerproxy.h \
    src/adblock/adblockmatcher.h \
    src/feedsview/feedsproxymodel.h \

SOURCES += \
    src/parseobject.cpp \
    src/optionsdialog.cpp \
    src/newsview/newsview.cpp \
    src/newsview/newsmodel.cpp \
    src/newsview/newsheader.cpp \
    src/aboutdialog.cpp \
    src/updateappdialog.cpp \
    src/feedpropertiesdialog.cpp \
    src/addfeedwizard.cpp \
    src/newstabwidget.cpp \
    src/findtext.cpp \
    src/findfeed.cpp \
    src/feedsview/feedsview.cpp \
    src/feedsview/feedsmodel.cpp \
    src/addfolderdialog.cpp \
    src/labeldialog.cpp \
    src/faviconobject.cpp \
    src/customizetoolbardialog.cpp \
    src/plugins/webpluginfactory.cpp \
    src/plugins/clicktoflash.cpp \
    src/downloads/downloadmanager.cpp \
    src/downloads/downloaditem.cpp \
    src/tabbar.cpp \
    src/categoriestreewidget.cpp \
    src/cleanupwizard.cpp \
    src/updatefeeds.cpp \
    src/requestfeed.cpp \
    src/notifications/notificationsfeeditem.cpp \
    src/notifications/notificationsnewsitem.cpp \
    src/notifications/notificationswidget.cpp \
    src/application/mainapplication.cpp \
    src/application/settings.cpp \
    src/application/logfile.cpp \
    src/application/mainwindow.cpp \
    src/main/main.cpp \
    src/adblock/adblocktreewidget.cpp \
    src/adblock/adblocksubscription.cpp \
    src/adblock/adblocksearchtree.cpp \
    src/adblock/adblockrule.cpp \
    src/adblock/adblockmanager.cpp \
    src/adblock/adblockicon.cpp \
    src/adblock/adblockdialog.cpp \
    src/adblock/adblockblockednetworkreply.cpp \
    src/adblock/adblockaddsubscriptiondialog.cpp \
    src/adblock/followredirectreply.cpp \
    src/application/splashscreen.cpp \
    src/network/authenticationdialog.cpp \
    src/network/cookiejar.cpp \
    src/network/networkmanager.cpp \
    src/webview/locationbar.cpp \
    src/webview/rssdetectionwidget.cpp \
    src/webview/webpage.cpp \
    src/webview/webview.cpp \
    src/database/database.cpp \
    src/common/common.cpp \
    src/common/delegatewithoutfocus.cpp \
    src/common/dialog.cpp \
    src/common/lineedit.cpp \
    src/common/toolbutton.cpp \
    src/newsfilters/filterrulesdialog.cpp \
    src/newsfilters/newsfiltersdialog.cpp \
    src/newsfilters/itemcondition.cpp \
    src/newsfilters/itemaction.cpp \
    src/network/sslerrordialog.cpp \
    src/network/networkmanagerproxy.cpp \
    src/adblock/adblockmatcher.cpp \
    src/feedsview/feedsproxymodel.cpp

INCLUDEPATH +=  $$PWD/src \
                $$PWD/src/application \
                $$PWD/src/common \
                $$PWD/src/main \
                $$PWD/src/database \
                $$PWD/src/downloads \
                $$PWD/src/feedsview \
                $$PWD/src/newsfilters \
                $$PWD/src/newsview \
                $$PWD/src/notifications \
                $$PWD/src/plugins \
                $$PWD/src/adblock \
                $$PWD/src/network \
                $$PWD/src/webview \

CONFIG += debug_and_release
CONFIG(debug, debug|release) {
  BUILD_DIR = $$OUT_PWD/debug
} else {
  BUILD_DIR = $$OUT_PWD/release
#  DEFINES += QT_NO_DEBUG_OUTPUT
}

DESTDIR = $${BUILD_DIR}/target
OBJECTS_DIR = $${BUILD_DIR}/obj
MOC_DIR = $${BUILD_DIR}/moc
RCC_DIR = $${BUILD_DIR}/rcc

isEmpty(SYSTEMQTSA) {
  include(3rdparty/qtsingleapplication/qtsingleapplication.pri)
} else {
  CONFIG += qtsingleapplication
}
isEqual(QT_MAJOR_VERSION, 5) {
  include(3rdparty/qftp/qftp.pri)
}
include(3rdparty/sqlite.pri)
include(lang/lang.pri)
include(3rdparty/qupzilla/qupzilla.pri)
include(3rdparty/ganalytics/ganalytics.pri)

os2|win32|mac {
  TARGET = QuiteRSS
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
  LIBS += -lUser32

  QMAKE_CXXFLAGS += -D__PRETTY_FUNCTION__=__FUNCTION__
  QMAKE_CFLAGS += -D__PRETTY_FUNCTION__=__FUNCTION__
}

os2 {
  RC_FILE = quiterss_os2.rc
}

os2|win32 {
  SOURCES += src/network/cabundleupdater.cpp
  HEADERS += src/network/cabundleupdater.h
  RESOURCES += data/ca-bundle.qrc
}

DISTFILES += \
    HISTORY_RU \
    HISTORY_EN \
    COPYING \
    AUTHORS \
    CHANGELOG \
    README.md

unix:!mac {
  TARGET = quiterss

  isEmpty(PREFIX) {
    PREFIX =   /usr/local
  }
  DATA_DIR = $$PREFIX/share/quiterss
  DEFINES += RESOURCES_DIR='\\\"$${DATA_DIR}\\\"'

  target.path =  $$quote($$PREFIX/bin)

  desktop.files = quiterss.desktop
  desktop.path =  $$quote($$PREFIX/share/applications)

  target1.files = images/48x48/quiterss.png
  target1.path =  $$quote($$PREFIX/share/pixmaps)

  icon_16.files =  images/16x16/quiterss.png
  icon_32.files =  images/32x32/quiterss.png
  icon_48.files =  images/48x48/quiterss.png
  icon_64.files =  images/64x64/quiterss.png
  icon_128.files = images/128x128/quiterss.png
  icon_256.files = images/256x256/quiterss.png
  icon_16.path =  $$quote($$PREFIX/share/icons/hicolor/16x16/apps)
  icon_32.path =  $$quote($$PREFIX/share/icons/hicolor/32x32/apps)
  icon_48.path =  $$quote($$PREFIX/share/icons/hicolor/48x48/apps)
  icon_64.path =  $$quote($$PREFIX/share/icons/hicolor/64x64/apps)
  icon_128.path = $$quote($$PREFIX/share/icons/hicolor/128x128/apps)
  icon_256.path = $$quote($$PREFIX/share/icons/hicolor/256x256/apps)

  translations.files = $$quote($$DESTDIR/lang)
  translations.path =  $$quote($$DATA_DIR)
  translations.CONFIG += no_check_exist

  sound.files = sound
  sound.path = $$quote($$DATA_DIR)

  style.files = style
  style.path = $$quote($$DATA_DIR)

  INSTALLS += target desktop target1
  INSTALLS += icon_16 icon_32 icon_48 icon_64 icon_128 icon_256
  INSTALLS += translations sound style
}

mac {
  CONFIG += app_bundle
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6

  QMAKE_INFO_PLIST = Info.plist
  ICON = quiterss.icns

  bundle_target.files += AUTHORS
  bundle_target.files += COPYING
  bundle_target.files += CHANGELOG
  bundle_target.files += README.md
  bundle_target.files += sound
  bundle_target.files += style
  bundle_target.path = Contents/Resources
  QMAKE_BUNDLE_DATA += bundle_target

  translations.files = $$quote($$DESTDIR/lang)
  translations.path =  Contents/Resources
  QMAKE_BUNDLE_DATA += translations

  INSTALLS += bundle_target translations
}

RESOURCES += \
    QuiteRSS.qrc

CODECFORTR  = UTF-8
CODECFORSRC = UTF-8

OTHER_FILES += \
    HISTORY_RU \
    HISTORY_EN \
    COPYING \
    AUTHORS \
    CHANGELOG \
    INSTALL \
    Info.plist

FORMS += \
    src/adblock/adblockdialog.ui \
    src/adblock/adblockaddsubscriptiondialog.ui
