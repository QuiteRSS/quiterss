QT += core gui network xml webkit sql

TARGET = QtRss
TEMPLATE = app

HEADERS += rsslisting.h \
    VersionNo.h
SOURCES += main.cpp rsslisting.cpp

RESOURCES += \
    qtrss.qrc
RC_FILE = QtRssApp.rc


