include( $${PWD}/../playground.pri )

TARGET = inputpanel

DEFINES += PLUGIN_PATH=$$clean_path( $$QSK_OUT_ROOT/plugins )

HEADERS += \
    LineEditSkinlet.h \
    LineEdit.h

SOURCES += \
    LineEditSkinlet.cpp \
    LineEdit.cpp \
    main.cpp
