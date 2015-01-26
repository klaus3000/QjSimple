DEFINES -= UNICODE
TEMPLATE = app
TARGET = QjSimple
QT += core \
    gui \
    xml \
    network
HEADERS += buddy.h \
    PjCallback.h \
    accountdialog.h \
    debugdialog.h \
    addbuddydialog.h \
    imwidget.h \
    qjsimple.h
SOURCES += buddy.cpp \
    PjCallback.cpp \
    accountdialog.cpp \
    debugdialog.cpp \
    addbuddydialog.cpp \
    imwidget.cpp \
    main.cpp \
    qjsimple.cpp
FORMS += accountdialog.ui \
    debugdialog.ui \
    addbuddydialog.ui \
    imwidget.ui \
    qjsimple.ui
RESOURCES += icons.qrc
win32-g++:RC_FILE = appicon.rc
INCLUDEPATH += ../pjproject-1.8.10/pjlib/include \
    ../pjproject-1.8.10/pjlib-util/include \
    ../pjproject-1.8.10/pjnath/include \
    ../pjproject-1.8.10/pjmedia/include \
    ../pjproject-1.8.10/pjsip/include
LIBS += -L../pjproject-1.8.10/pjlib/lib \
    -L../pjproject-1.8.10/pjlib-util/lib \
    -L../pjproject-1.8.10/pjnath/lib \
    -L../pjproject-1.8.10/pjmedia/lib \
    -L../pjproject-1.8.10/pjsip/lib \
    -L../pjproject-1.8.10/third_party/lib

# INCLUDEPATH += ../pjproject-1.5.5/pjlib/include \
# ../pjproject-1.5.5/pjlib-util/include \
# ../pjproject-1.5.5/pjnath/include \
# ../pjproject-1.5.5/pjmedia/include \
# ../pjproject-1.5.5/pjsip/include
# LIBS += -L../pjproject-1.5.5/pjlib/lib \
# -L../pjproject-1.5.5/pjlib-util/lib \
# -L../pjproject-1.5.5/pjnath/lib \
# -L../pjproject-1.5.5/pjmedia/lib \
# -L../pjproject-1.5.5/pjsip/lib \
# -L../pjproject-1.5.5/third_party/lib
# INCLUDEPATH += ../pjproject-1.4/pjlib/include \
# ../pjproject-1.4/pjlib-util/include \
# ../pjproject-1.4/pjnath/include \
# ../pjproject-1.4/pjmedia/include \
# ../pjproject-1.4/pjsip/include
# LIBS += -L../pjproject-1.4/pjlib/lib \
# -L../pjproject-1.4/pjlib-util/lib \
# -L../pjproject-1.4/pjnath/lib \
# -L../pjproject-1.4/pjmedia/lib \
# -L../pjproject-1.4/pjsip/lib \
# -L../pjproject-1.4/third_party/lib
# win32-g++:LIBS += -L../openssl-0.9.8g
win32-g++:LIBS += -L../OpenSSL/lib/mingw
win32-g++:LIBS += -lpjsua-i686-pc-mingw32 \
    -lpjsip-ua-i686-pc-mingw32 \
    -lpjsip-simple-i686-pc-mingw32 \
    -lpjsip-i686-pc-mingw32 \
    -lpjmedia-codec-i686-pc-mingw32 \
    -lpjmedia-i686-pc-mingw32 \
    -lpjmedia-codec-i686-pc-mingw32 \
    -lpjmedia-audiodev-i686-pc-mingw32 \
    -lpjnath-i686-pc-mingw32 \
    -lpjlib-util-i686-pc-mingw32 \
    -lpj-i686-pc-mingw32 \
    -lportaudio-i686-pc-mingw32 \
    -lgsmcodec-i686-pc-mingw32 \
    -lilbccodec-i686-pc-mingw32 \
    -lspeex-i686-pc-mingw32 \
    -lresample-i686-pc-mingw32 \
    -lmilenage-i686-pc-mingw32 \
    -lsrtp-i686-pc-mingw32 \
    -lm \
    -lwinmm \
    -lole32 \
    -lws2_32 \
    -lwsock32 \
    -lssl \
    -lcrypto \
    -lgdi32
linux-g++:LIBS += -lpjsua-i686-pc-linux-gnu \
    -lpjsip-ua-i686-pc-linux-gnu \
    -lpjsip-simple-i686-pc-linux-gnu \
    -lpjsip-i686-pc-linux-gnu \
    -lpjmedia-codec-i686-pc-linux-gnu \
    -lpjmedia-i686-pc-linux-gnu \
    -lpjmedia-codec-i686-pc-linux-gnu \
    -lpjmedia-audiodev-i686-pc-linux-gnu \
    -lpjnath-i686-pc-linux-gnu \
    -lpjlib-util-i686-pc-linux-gnu \
    -lpj-i686-pc-linux-gnu \
    -lportaudio-i686-pc-linux-gnu \
    -lgsmcodec-i686-pc-linux-gnu \
    -lilbccodec-i686-pc-linux-gnu \
    -lspeex-i686-pc-linux-gnu \
    -lresample-i686-pc-linux-gnu \
    -lmilenage-i686-pc-linux-gnu \
    -lsrtp-i686-pc-linux-gnu \
    -lm \
    -lpthread \
    -lssl \
    -lasound \
    -luuid
linux-g++-64:LIBS += -lpjsua-x86_64-unknown-linux-gnu \
    -lpjsip-ua-x86_64-unknown-linux-gnu \
    -lpjsip-simple-x86_64-unknown-linux-gnu \
    -lpjsip-x86_64-unknown-linux-gnu \
    -lpjmedia-codec-x86_64-unknown-linux-gnu \
    -lpjmedia-x86_64-unknown-linux-gnu \
    -lpjmedia-codec-x86_64-unknown-linux-gnu \
    -lpjmedia-audiodev-x86_64-unknown-linux-gnu \
    -lpjnath-x86_64-unknown-linux-gnu \
    -lpjlib-util-x86_64-unknown-linux-gnu \
    -lpj-x86_64-unknown-linux-gnu \
    -lportaudio-x86_64-unknown-linux-gnu \
    -lgsmcodec-x86_64-unknown-linux-gnu \
    -lilbccodec-x86_64-unknown-linux-gnu \
    -lspeex-x86_64-unknown-linux-gnu \
    -lresample-x86_64-unknown-linux-gnu \
    -lmilenage-x86_64-unknown-linux-gnu \
    -lsrtp-x86_64-unknown-linux-gnu \
    -lm \
    -lpthread \
    -lssl \
    -lasound \
    -luuid
macx-g++:LIBS += -lpjsua-i386-apple-darwin9.7.1 \
    -lpjsip-ua-i386-apple-darwin9.7.1 \
    -lpjsip-simple-i386-apple-darwin9.7.1 \
    -lpjsip-i386-apple-darwin9.7.1 \
    -lpjmedia-i386-apple-darwin9.7.1 \
    -lpjmedia-i386-apple-darwin9.7.1 \
    -lpjmedia-codec-i386-apple-darwin9.7.1 \
    -lpjmedia-audiodev-i386-apple-darwin9.7.1 \
    -lpjnath-i386-apple-darwin9.7.1 \
    -lpjlib-util-i386-apple-darwin9.7.1 \
    -lpj-i386-apple-darwin9.7.1 \
    -lportaudio-i386-apple-darwin9.7.1 \
    -lgsmcodec-i386-apple-darwin9.7.1 \
    -lilbccodec-i386-apple-darwin9.7.1 \
    -lspeex-i386-apple-darwin9.7.1 \
    -lresample-i386-apple-darwin9.7.1 \
    -lmilenage-i386-apple-darwin9.7.1 \
    -lsrtp-i386-apple-darwin9.7.1 \
    -lm \
    -lpthread \
    -lssl \
    -lcrypto \
    -framework \
    CoreAudio \
    -framework \
    AudioToolbox \
    -framework \
    AudioUnit
OTHER_FILES += Changelog
