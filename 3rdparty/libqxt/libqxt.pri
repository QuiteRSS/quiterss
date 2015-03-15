INCLUDEPATH += $$PWD

DEFINES += QXT_STATIC

HEADERS += \
    $$PWD/qxtglobal.h \
    $$PWD/qxtglobalshortcut.h \
    $$PWD/qxtglobalshortcut_p.h
SOURCES += \
    $$PWD/qxtglobal.cpp \
    $$PWD/qxtglobalshortcut.cpp \

unix:!macx {
    SOURCES += $$PWD/qxtglobalshortcut_x11.cpp
}
macx {
    SOURCES += $$PWD/qxtglobalshortcut_mac.cpp
}
win32 {
    SOURCES += $$PWD/qxtglobalshortcut_win.cpp
}
