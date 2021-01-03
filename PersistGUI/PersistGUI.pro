ICON    = Icon.icns
QT      += core gui widgets
CONFIG  += c++17
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

INCLUDEPATH += ../src /usr/local/include
DEPENDPATH += $$PWD/../../../../../usr/local/include

SOURCES += \
    Configuration.cpp \
    TableForm.cpp \
    main.cpp \
	MainWindow.cpp \
	../src/DataModel.cpp

HEADERS += \
    Configuration.h \
	MainWindow.h \
	../src/DataModel.h \
    TableForm.h

FORMS += \
    MainWindow.ui \
    TableForm.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/release/ -lshow-mac
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../../usr/local/lib/debug/ -lshow-mac
else:unix: LIBS += -L$$PWD/../../../../../usr/local/lib/ -lshow-mac

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../usr/local/lib/release/libshow-mac.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../usr/local/lib/debug/libshow-mac.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../usr/local/lib/release/show-mac.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$PWD/../../../../../usr/local/lib/debug/show-mac.lib
else:unix: PRE_TARGETDEPS += $$PWD/../../../../../usr/local/lib/libshow-mac.a
