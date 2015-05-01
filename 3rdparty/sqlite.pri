#DEFINES += SQLITEDRIVER_DEBUG
#DEFINES += SQLITEDRIVER_EXPORT

INCLUDEPATH += $$PWD \
               $$PWD/sqlitex

os2|win32|mac {
  CONFIG(release, debug|release):DEFINES *= NDEBUG
  DEFINES += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE

  HEADERS +=      $$PWD/sqlite/sqlite3.h
  SOURCES +=      $$PWD/sqlite/sqlite3.c

  INCLUDEPATH += $$PWD/sqlite
} else {
  CONFIG += link_pkgconfig
  PKGCONFIG += sqlite3
}

HEADERS += $$PWD/sqlitex/sqlcachedresult.h \
           $$PWD/sqlitex/sqlitedriver.h \
           $$PWD/sqlitex/sqliteextension.h

SOURCES += $$PWD/sqlitex/sqlcachedresult.cpp \
           $$PWD/sqlitex/sqlitedriver.cpp \
           $$PWD/sqlitex/sqliteextension.cpp
