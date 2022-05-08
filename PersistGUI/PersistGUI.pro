ICON    = Icon.icns
QT      += core gui widgets
CONFIG  += c++17
CONFIG  += sdk_no_version_check

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

INCLUDEPATH += ../src /usr/local/include
DEPENDPATH += /usr/local/include

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

win32 {
        LIBS += -lshow-win
}
macx {
        LIBS += -lshow-mac
}
unix {
        LIBS += -lshow
}
