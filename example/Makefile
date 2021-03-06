#
# The examples Makefile. Assumes DataModeler is in the parent directory.
#

UNAME := $(shell uname)
MACAPPEND=
AUTO_ARGUMENT=

ifeq ($(UNAME), Darwin)
	MACAPPEND=-mac
endif


DATAMODELER := ../DataModeler
DEPDIR := .d
OBJDIR := obj
BINDIR := bin
GENDIR := gensrc

VPATH := .:${GENDIR}:${GENDIR}/stubs

# This gives us some starter values. Makefiles should use += to add to these.
# Be careful with CXX flags and use = instead of := so that INCLUDES gets late expansion.
INCLUDES := -I${GENDIR} -I${GENDIR}/stubs -I/usr/local/include
CXXFLAGS = ${INCLUDES} --std=c++17 -g -Wno-unused-local-typedefs -Wno-deprecated-declarations ${AUTO_ARGUMENT}
LDFLAGS := -L/usr/local/lib -lshow${MACAPPEND} -lfaker -lboost_filesystem -lpqxx -lpq

DEPFLAGS = -MT $@ -MMD -MP -MF $(DEPDIR)/$*.Td

COMPILE.c = $(CC) $(DEPFLAGS) ${INCLUDES} $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
COMPILE.cc = $(CXX) $(DEPFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c
POSTCOMPILE = @mv -f $(DEPDIR)/$*.Td $(DEPDIR)/$*.d && touch $@


all: directories db.sql ${BINDIR}/main

.PHONY: directories
directories: ${DEPDIR} ${OBJDIR} ${BINDIR} ${GENDIR}/stubs

${DEPDIR}:
		mkdir -p $@

${OBJDIR}:
		mkdir -p $@

${BINDIR}:
		mkdir -p $@

${GENDIR}/stubs:
		mkdir -p $@

clean:
		rm -rf ${DEPDIR} ${OBJDIR} ${GENDIR} db.sql

generate:
		${DATAMODELER} --generate --model db.json  --cppdir ${GENDIR} --stubdir ${GENDIR}/stubs

#
# This actually generates everything.
#
db.sql: db.json
		${DATAMODELER} --generate --model db.json  --cppdir ${GENDIR} --stubdir ${GENDIR}/stubs

OBJS := ${OBJDIR}/main.o \
				${OBJDIR}/Foo.o ${OBJDIR}/Foo_Base.o ${OBJDIR}/DB_Foo.o \
				${OBJDIR}/Bar.o ${OBJDIR}/Bar_Base.o ${OBJDIR}/DB_Bar.o
${BINDIR}/main: ${OBJS}
		$(CXX) ${OBJS} ${LDFLAGS} $(OUTPUT_OPTION)

#======================================================================
# How to make the .o files as. This is a little complicated but is
# derived from:
# http://make.mad-scientist.net/papers/advanced-auto-dependency-generation/
#======================================================================

${OBJDIR}/%.o : %.cpp
	$(COMPILE.cc) $(OUTPUT_OPTION) $<
	$(POSTCOMPILE)

$(DEPDIR)/%.d: ;
.PRECIOUS: $(DEPDIR)/%.d

INCLUDELIST = $(wildcard $(patsubst %,$(DEPDIR)/%.d,$(basename $(SRCS_NODIR))))
include ${INCLUDELIST}
