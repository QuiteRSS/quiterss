CONFIG(release, debug|release):DEFINES *= NDEBUG
DEFINES += SQLITE_OMIT_LOAD_EXTENSION SQLITE_OMIT_COMPLETE

QT_VERSION = $$[QT_VERSION]
QT_VERSION = $$split(QT_VERSION, ".")
QT_VER_MAJ = $$member(QT_VERSION, 0)
QT_VER_MIN = $$member(QT_VERSION, 1)

lessThan(QT_VER_MAJ, 4) | lessThan(QT_VER_MIN, 8) {
INCLUDEPATH +=  $$PWD/sqlite_qt47x
SOURCES +=      $$PWD/sqlite_qt47x/sqlite3.c
} else {
INCLUDEPATH +=  $$PWD/sqlite
SOURCES +=      $$PWD/sqlite/sqlite3.c
}
