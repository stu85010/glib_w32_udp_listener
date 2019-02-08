# https://zrj.me/archives/952
# GIT_DESCRIBE = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags)
# GIT_HASH7    = $$system(git --git-dir $$PWD/.git --work-tree $$PWD describe --always)
# GIT_HASH     = $$system(git rev-parse HEAD)
gitver.h:
	@printf '#ifndef GIT_HASH\n#define GIT_HASH "' > $@ && \
	git log -1 --pretty=format:"%H" | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@ && \
	printf '\n#ifndef GIT_HASH7\n#define GIT_HASH7 "' >> $@ && \
	git log -1 --pretty=format:"%h" | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@ && \
	printf '\n#ifndef GIT_TAG\n#define GIT_TAG "' >> $@ && \
	git --git-dir $$PWD/.git --work-tree $$PWD describe --always --tags | tr -d "\n" >> $@ && \
	printf '"\n#endif\n' >> $@ && \
	printf '\n#ifndef MAKE_TIME\n'>> $@ && \
	printf '#define MAKE_TIME "' | tr -d "\n">> $@ && \
	date +%FT%T%Z | tr -d "\n">> $@ && \
	printf '"\n#endif\n' >> $@

genver: gitver.h


OSFLAG :=
ifeq ($(OS),Windows_NT)
	OSFLAG += -D WIN32
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
		OSFLAG += -D AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
		OSFLAG += -D IA32
	endif
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Linux)
		OSFLAG += -D LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
		OSFLAG += -D OSX
	endif
		UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
		OSFLAG += -D AMD64
	endif
		ifneq ($(filter %86,$(UNAME_P)),)
	OSFLAG += -D IA32
		endif
	ifneq ($(filter arm%,$(UNAME_P)),)
		OSFLAG += -D ARM
	endif
endif

###############################################################################
#
# Generic Makefile for C/C++ Program
#
# Author: whyglinux (whyglinux AT hotmail DOT com)
# Date: 2006/03/04

# Description:
# The makefile searches in directories for the source files
# with extensions specified in , then compiles the sources
# and finally produces the , the executable file, by linking
# the objectives.

# Usage:
# $ make compile and link the program.
# $ make objs compile only (no linking. Rarely used).
# $ make clean clean the objectives and dependencies.
# $ make cleanall clean the objectives, dependencies and executable.
# $ make rebuild rebuild the program. The same as make clean && make all.
#==============================================================================

## Customizing Section: adjust the following if necessary.
##=============================================================================

# The executable file name.
# It must be specified.
# PROGRAM := a.out # the executable name
PROGRAM		:= glib-udp-listener
TARGETDIR   = bin
OBJDIR      := $(TARGETDIR)/obj

# The directories in which source files reside.
# At least one path should be specified.
# SRCDIRS := . # current directory
SRCDIRS :=  \
		   .

INCDIRS  :=  \
		   .

# The source file types (headers excluded).
# At least one type should be specified.
# The valid suffixes are among of .c, .C, .cc, .cpp, .CPP, .c++, .cp, or .cxx.
# SRCEXTS := .c # C program
# SRCEXTS := .cpp # C++ program
# SRCEXTS := .c .cpp # C/C++ program
SRCEXTS := .c .cpp

# The flags used by the cpp (man cpp for more).
# CPPFLAGS := -Wall -Werror # show all warnings and take them as errors
# CPPFLAGS :=

ifneq (,$(findstring OSX,$(OSFLAG)))
# Found
INCDIRS += \
	-I/usr/include \
	-I/usr/local/include \
	-I/usr/local/Cellar/glib/2.50.2/include/glib-2.0 \
	-I/usr/local/Cellar/glib/2.50.2/lib/glib-2.0/include \
	-I/usr/local/opt/gettext/include \
	-I/usr/local/Cellar/pcre/8.42/include

LDFLAGS += \
	-L/usr/local/lib

else ifneq (,$(findstring LINUX,$(OSFLAG)))
# Found
# INCDIRS += -I/usr/include
# LDFLAGS += -L/usr/lib
LDFLAGS += -lstdc++
endif

# The compiling flags used only for C.
# If it is a C++ program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C parts.
# CFLAGS :=
CFLAGS += $(INCDIRS)

# The compiling flags used only for C++.
# If it is a C program, no need to set these flags.
# If it is a C and C++ merging program, set these flags for the C++ parts.
# CXXFLAGS := -std=c++11
CXXFLAGS += -std=c++11 $(INCDIRS)

# The library and the link options ( C and C++ common).
# LDFLAGS :=
LDFLAGS += \
			-lgio-2.0 \
			-lgobject-2.0 \
			-lgthread-2.0 \
			-lglib-2.0



## Implict Section: change the following only when necessary.
##=============================================================================
# The C program compiler. Uncomment it to specify yours explicitly.
#CC = gcc

# The C++ program compiler. Uncomment it to specify yours explicitly.
#CXX = g++

# Uncomment the 2 lines to compile C programs as C++ ones.
#CC = $(CXX)
#CFLAGS = $(CXXFLAGS)

# The command used to delete file.
#RM = rm -f

## Stable Section: usually no need to be changed. But you can add more.
##=============================================================================
SHELL = /bin/sh
_SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
# excluded prefix with 'test_'
SOURCES = $(foreach a,$(_SOURCES),$(if $(findstring test_,$a),,$a))
OBJS = $(foreach x,$(SRCEXTS), \
$(patsubst %$(x),$(OBJDIR)/%.o,$(filter %$(x),$(SOURCES))))
# OBJECTS     = $(SOURCES:%.c=$(OBJDIR)/%.o)
DEPS = $(patsubst %.o,%.d,$(OBJS))

# Objects for UnitTest (remove main entry)
TEST_SOURCES = $(foreach d,$(SRCDIRS),$(wildcard $(addprefix $(d)/*,$(SRCEXTS))))
# excluded prefix with 'test_'
_TEST_OBJS = $(foreach x,$(SRCEXTS), \
$(patsubst %$(x),$(OBJDIR)/%.o,$(filter %$(x),$(TEST_SOURCES))))
TEST_OBJS := $(filter-out $(OBJDIR)/module/fcgi_agent.o,$(_TEST_OBJS))

.PHONY : all objs clean cleanall rebuild status

all : $(PROGRAM)

# Rules for creating the dependency files (.d).
#---------------------------------------------------
%.d : %.c
	@$(CC) -MM -MD $(CFLAGS) $<

%.d : %.C
	@$(CC) -MM -MD $(CXXFLAGS) $<

%.d : %.cc
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.d : %.cpp
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.d : %.CPP
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.d : %.c++
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.d : %.cp
	@$(CXX) -MM -MD $(CXXFLAGS) $<

%.d : %.cxx
	@$(CXX) -MM -MD $(CXXFLAGS) $<

# Rules for producing the objects.
#---------------------------------------------------
objs : genver create_directories
	$(MAKE) __obj_files $(MAKEFLAGS)

__obj_files : $(OBJS)
	$(info [Objects generated])

$(OBJDIR)/%.o : %.c
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<)
	@$(CC) -c $(CPPFLAGS) $(CFLAGS) -o $@ $<

$(OBJDIR)/%.o : %.C
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o : %.cc
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(OBJDIR)/%.o : %.cpp
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) -o $@ $<

$(OBJDIR)/%.o : %.CPP
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(OBJDIR)/%.o : %.c++
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX -c $(CPPFLAGS) $(CXXFLAGS) $<)
	@$(CXX -c $(CPPFLAGS) $(CXXFLAGS) $<

$(OBJDIR)/%.o : %.cp
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

$(OBJDIR)/%.o : %.cxx
	@mkdir -p $(@D)
	$(info Compiling... $<: $(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<)
	@$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $<

# Rules for producing the executable.
__binary: $(OBJS)
	$(info Linking for exec for $(PROGRAM)...)
ifeq ($(strip $(SRCEXTS)), .c) # C file
	$(CC) -o $(TARGETDIR)/$(PROGRAM) $(OBJS) $(LDFLAGS)
else # C++ file
	$(CXX) -o $(TARGETDIR)/$(PROGRAM) $(OBJS) $(LDFLAGS)
endif
	$(MAKE) postbuild
	$(info [Created executable])

#----------------------------------------------
$(PROGRAM): genver create_directories
	$(info make flags $(MAKEFLAGS))
	$(MAKE) __binary $(MAKEFLAGS)

retest:
	@$(MAKE) clean $(MAKEFLAGS)
	@$(MAKE) test $(MAKEFLAGS)

test: $(TEST_OBJS)
	$(info UnitTest: Linking for exec... $(PROGRAM) : $(CC) -o $(TARGETDIR)/$(PROGRAM) $(TEST_OBJS) $(LDFLAGS))
ifeq ($(strip $(SRCEXTS)), .c) # C file
	@$(CC) -o $(TARGETDIR)/test_$(PROGRAM) $(TEST_OBJS) $(LDFLAGS)
else # C++ file
	@$(CXX) -o $(TARGETDIR)/test_$(PROGRAM) $(TEST_OBJS) $(LDFLAGS)
endif
	$(MAKE) postbuild
	$(info UnitTest: [Created executable])
	$(info UnitTest: Linking for exec... $(PROGRAM) : $(CC) -o $(TARGETDIR)/test_$(PROGRAM) $(TEST_OBJS) $(LDFLAGS))
	./$(TARGETDIR)/test_$(PROGRAM)

runtest:
	./$(TARGETDIR)/test_$(PROGRAM) &

-include $(DEPS)

postbuild:
	$(info [post build section])

rebuild:
	@$(MAKE) clean $(MAKEFLAGS)
	@$(MAKE) all $(MAKEFLAGS)

clean :
	@rm -rf $(OBJDIR)
	@$(RM) -r *.o *.d
	@rm -f gitver.h
	$(info [Cleaned objects])

cleanall: clean
	@$(RM) $(TARGETDIR)/*
	$(info [Cleaned all])

create_directories:
	@mkdir -p $(OBJDIR)
	$(info [Created directories])

status:
	$(info MAKEFLAGS: $(MAKEFLAGS))
	$(info CC: $(CC))
	$(info CXX: $(CXX))
	$(info CFLAGS: $(CFLAGS))
	$(info CXXFLAGS: $(CXXFLAGS))
	$(info LDFLAGS: $(LDFLAGS))
	$(info src_files: $(SOURCES))
	$(info obj_files: $(OBJS))
	$(info dep_files: $(DEPS))
	$(info objdir: $(OBJDIR))
	$(info target location: $(TARGETDIR)/$(TARGET_NAME))
	$(info OS: $(OS))
	$(info OSFLAG: $(OSFLAG))

check_format:
	$(call colorecho,'Checking formatting with astyle')
	./tools/check_format.sh
	# @cd "$(SRC_DIR)" && git diff --check

format:
	$(call colorecho,'Checking formatting with astyle')
	@./tools/check_format.sh --fix
	

run:
	./bin/glib-udp-listener
