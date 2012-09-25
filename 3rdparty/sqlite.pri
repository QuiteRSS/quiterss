CONFIG(release, debug|release):DEFINES *= NDEBUG
DEFINES += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE

lessThan(QT_MAJOR_VERSION, 4) | lessThan(QT_MINOR_VERSION, 8) {
INCLUDEPATH +=  $$PWD/sqlite_qt47x
HEADERS +=      $$PWD/sqlite_qt47x/sqlite3.h
SOURCES +=      $$PWD/sqlite_qt47x/sqlite3.c
} else {
INCLUDEPATH +=  $$PWD/sqlite
HEADERS +=      $$PWD/sqlite/sqlite3.h
SOURCES +=      $$PWD/sqlite/sqlite3.c
}

