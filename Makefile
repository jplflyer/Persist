#======================================================================
# GNU MAKE REQUIRED!
#
# Notice VPATH. This is a GNU Make thing that lets us put our source
# files where we want. I've broken them based on where they're running.
# I may later break them up even more for one directory for each program.
# Anyway, we search for .cpp files in the list of directories, and it
# just magically works.
#======================================================================

# This needs to be above the include, as he has targets.
.PHONY: all
all: directories DataModeler

# We include a standard base with lots of boilerplate.
include ../ShowLib/Makefile-Base

TEST_SRC=tests
TEST_BIN=test-bin${MACAPPEND}

VPATH := ${SRCDIR}:${TEST_SRC}
INCLUDES += -I../ShowLib/include -I../ShowLib
LDFLAGS += -L. -L../ShowLib/lib -lshow${MACAPPEND} -lz -llog4cplus -lcppunit -lpthread -lstdc++ -lm -ldl

SRC_NOSORT := $(wildcard src/*.cpp)
SRC_SORTED := $(sort ${SRC_NOSORT})
OBJS := $(patsubst src/%.cpp,${OBJDIR}/%.o,${SRC_SORTED})
OBJS_NOMAIN := $(patsubst ${OBJDIR}/main.o,,${OBJS})

#======================================================================
# Top-level targets.
#======================================================================

Makefile: ;

# Clean the contents of the subdirs.
.PHONY: clean
clean:
	rm -f ${DEPDIR}/* ${OBJDIR}/* ${LIB} ${TEST_BIN}/*

# Clean out the subdirs entirely.
.PHONY: reallyclean
reallyclean:
	rm -rf ${DEPDIR} ${OBJDIR}

.PHONY: echo
echo:
	@echo CXXFLAGS is ${CXXFLAGS}
	@echo SRC_SORTED is ${SRC_SORTED}
	@echo OBJS is ${OBJS}
	@echo OBJS_NOMAIN is ${OBJS_NOMAIN}
	@echo VPATH = ${VPATH}
	@echo LDFLAGS = ${LDFLAGS}

include .d/*

#======================================================================
# How to make the data modeler.
#======================================================================
DataModeler: ${OBJS}
	$(CXX) ${OBJS} -lshow${MACAPPEND} ${LDFLAGS} $(OUTPUT_OPTION)

#======================================================================
# Tests
#======================================================================

tests: ${TEST_BIN} ${TEST_BIN}/TestDataModel

${TEST_BIN}:
	mkdir -p ${TEST_BIN}

${TEST_BIN}/TestDataModel: ${OBJDIR}/TestDataModel.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN}
	$(CXX) ${OBJDIR}/TestDataModel.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN} -lshow${MACAPPEND} ${LDFLAGS} $(OUTPUT_OPTION)
