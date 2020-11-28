TEMPLATE = app

CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += sdk_no_version_check

INCLUDEPATH += ./src ../ShowLib/include ../ShowLib /usr/local/include

SOURCES += \
    src/DataModel.cpp \
    src/Processor.cpp \
    src/main.cpp \
    tests/TestDataModel.cpp \
    tests/main-test.cpp

HEADERS += \
    src/DataModel.h \
    src/Processor.h \
    tests/TestDataModel.h \
    tests/UnitTesting.h \
    tests/main-test.h
