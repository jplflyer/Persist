TEMPLATE = app

CONFIG += console c++20
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += sdk_no_version_check
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.17

INCLUDEPATH += ./src
INCLUDEPATH += /usr/local/include

SOURCES += \
    src/CodeGenerator.cpp \
    src/CodeGenerator_CPP.cpp \
    src/CodeGenerator_DB.cpp \
    src/CodeGenerator_Flyway.cpp \
    src/CodeGenerator_Java.cpp \
    src/CodeGenerator_SQL.cpp \
    src/DataModel.cpp \
    src/Processor.cpp \
    src/main.cpp \
    tests/TestDataModel.cpp \
    tests/TestDatabase.cpp \
    tests/main-test.cpp

HEADERS += \
    src/CodeGenerator.h \
    src/CodeGenerator_CPP.h \
    src/CodeGenerator_DB.h \
    src/CodeGenerator_Flyway.h \
    src/CodeGenerator_Java.h \
    src/CodeGenerator_SQL.h \
    src/DataModel.h \
    src/Processor.h \
    tests/TestDataModel.h \
    tests/TestDatabase.h \
    tests/UnitTesting.h \
    tests/main-test.h

FORMS +=
