TEMPLATE = app

CONFIG += console c++17
CONFIG -= app_bundle
CONFIG -= qt
CONFIG += sdk_no_version_check
QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

INCLUDEPATH += ./src
INCLUDEPATH += example example/gensrc

SOURCES += \
    example/gensrc/Bar.cpp \
    example/gensrc/DB_Bar.cpp \
    example/gensrc/DB_Foo.cpp \
    example/gensrc/Foo.cpp \
    example/main.cpp \
    src/CodeGenerator.cpp \
    src/CodeGenerator_CPP.cpp \
    src/CodeGenerator_DB.cpp \
    src/CodeGenerator_SQL.cpp \
    src/DataModel.cpp \
    src/Processor.cpp \
    src/main.cpp \
    tests/TestDataModel.cpp \
    tests/TestDatabase.cpp \
    tests/main-test.cpp

HEADERS += \
    example/gensrc/Bar.h \
    example/gensrc/DB_Bar.h \
    example/gensrc/DB_Foo.h \
    example/gensrc/Foo.h \
    src/CodeGenerator.h \
    src/CodeGenerator_CPP.h \
    src/CodeGenerator_DB.h \
    src/CodeGenerator_SQL.h \
    src/DataModel.h \
    src/Processor.h \
    tests/TestDataModel.h \
    tests/TestDatabase.h \
    tests/UnitTesting.h \
    tests/main-test.h
