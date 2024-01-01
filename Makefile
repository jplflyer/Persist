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
all: directories makelib DataModeler

# We include a standard base with lots of boilerplate.
include ../ShowLib/Makefile-Base

TEST_SRC=tests
TEST_BIN=test-bin${MACAPPEND}

VPATH := ${SRCDIR}:${TEST_SRC}
INCLUDES += -I../ShowLib/include -I../ShowLib
LDFLAGS += -L. -L../ShowLib/lib -lshow${MACAPPEND} -lz -llog4cplus -lpthread -lm -ldl

SRC_NOSORT := $(wildcard src/*.cpp)
SRC_SORTED := $(sort ${SRC_NOSORT})
OBJS := $(patsubst src/%.cpp,${OBJDIR}/%.o,${SRC_SORTED})
OBJS_NOMAIN := $(patsubst ${OBJDIR}/main.o,,${OBJS})

CORES = $(shell lscpu | egrep Core.s | cut -d: -f2 | sed 's/ //g')

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
	@echo CORES is ${CORES}

#
# This target gets called from make all. It is used to
# do -j10 while making the .o files -- speeding it up
# rather dramatically.
#
# If you just set MAKEFLAGS += -j10, and you do "make clean all",
# the parallelism doesn't work. It tries to make the binary because
# it doesn't realize that the .o list was cleared out by the clean
# target.
#
makelib:
	@$(MAKE) ${THREADING_ARG} --output-sync=target --no-print-directory objects

objects: ${OBJS}

#======================================================================
# How to make the data modeler.
#======================================================================
DataModeler: ${OBJS}
	$(CXX) ${OBJS} ${LDFLAGS} $(OUTPUT_OPTION)

#======================================================================
# Install
#======================================================================
.PHONY: install
install: /usr/local/bin/DataModeler
/usr/local/bin/DataModeler: DataModeler
		cp DataModeler /usr/local/bin/DataModeler


#======================================================================
# Tests
#======================================================================

tests: ${TEST_BIN} ${TEST_BIN}/TestDataModel ${TEST_BIN}/TestDatabase

${TEST_BIN}:
	mkdir -p ${TEST_BIN}

${TEST_BIN}/TestDataModel: ${OBJDIR}/TestDataModel.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN}
	$(CXX) ${OBJDIR}/TestDataModel.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN} -lshow${MACAPPEND} ${LDFLAGS} $(OUTPUT_OPTION)

${TEST_BIN}/TestDatabase: ${OBJDIR}/TestDatabase.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN}
	$(CXX) ${OBJDIR}/TestDatabase.o ${OBJDIR}/main-test.o ${OBJS_NOMAIN} -lshow${MACAPPEND} -lpqxx -lpq ${LDFLAGS} $(OUTPUT_OPTION)
