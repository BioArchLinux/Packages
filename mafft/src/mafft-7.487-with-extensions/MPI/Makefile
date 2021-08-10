PREFIX = /usr/local
LIBDIR = $(PREFIX)/libexec/mafft
BINDIR = $(PREFIX)/bin
MANDIR = $(PREFIX)/share/man/man1
DESTDIR = 


#CC :=  mpicc -cc=gcc
CC :=  mpicc
#CC :=  /u33/app/openmpi/bin/mpicc
#CFLAGS := -O3 -ipo -no-prec-div -xHost -std=c99
#CFLAGS := -O3 -std=c99
CFLAGS := -O3
#CFLAGS =  -O0  -fPIC -pedantic -Wall -std=c99 -g -DMALLOC_CHECK_=3 # for shark, valgrind
# CFLAGS := -O0 -g -ipo -no-prec-div -xHost -std=c99
TARGET_ARCH := 
LDLIBS := -lm -lrt -lpthread  # -lrt?


ENABLE_MULTITHREAD := -Denablemultithread
#ENABLE_ATOMIC := -Denableatomic
VPATH := ../core

ifdef ENABLE_ATOMIC
STDF = -std=c11
else
STDF = -std=c99
endif

MYCFLAGS := $(ENABLE_MULTITHREAD) $(ENABLE_ATOMIC) $(STDF) $(CFLAGS) -I../core -MMD -MP

OBJDIR := ./obj
ifeq "$(strip $(OBJDIR))" ""
  OBJDIR := .
endif

INSTALL = install

PROGS := nodepair_mpi
SCRIPTS := mpiscript
OBJNODPAIR := mtxutl.o io.o mltaln9_mpi.o tddis.o constants.o MSalignmm.o partSalignmm.o Lalignmm.o rna.o Salignmm.o Dalignmm.o \
		    Falign.o Falign_localhom.o Galign11.o Lalign11.o genalign11.o SAalignmm.o \
			nodepair_mpi.o defs.o fft.o fftFunctions.o addfunctions.o \
			pairlocalalign_mpi.o MSalign11.o
OBJNODPAIR := $(addprefix $(OBJDIR)/, $(OBJNODPAIR))
DEPSNODPAIR := $(OBJNODPAIR:%.o=%.d) 


all : $(PROGS) $(SCRIPTS)
	cp $(PROGS) $(SCRIPTS) ../binaries
	chmod 755 ../binaries/*
	@echo done.

install : all
	mkdir -p $(DESTDIR)$(LIBDIR)
	chmod 755 $(DESTDIR)$(LIBDIR)
	chmod 755 $(PROGS) ||:     # in MinGW, it's ok if this fails
	$(INSTALL) -s $(PROGS) $(DESTDIR)$(LIBDIR)
	$(INSTALL) $(SCRIPTS) $(DESTDIR)$(LIBDIR)
	@echo done.

nodepair_mpi : $(OBJNODPAIR)
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

shmclear : shmclear.c
	$(CC) $(LDFLAGS) $(TARGET_ARCH) $^ $(LOADLIBES) $(LDLIBS) -o $@

mpiscript : mpiscript.tmpl
	cp mpiscript.tmpl mpiscript

$(OBJDIR)/%.o : %.c
	-@mkdir -p $(OBJDIR)
	@[ -d $(OBJDIR) ]
	$(CC) $(MYCFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -o $@ -c $<

clean :
	rm -f $(OBJNODPAIR) $(DEPSNODPAIR) $(PROGS) $(SCRIPTS) *.o *.a *.exe *~
	rm -rf obj/

-include $(DEPSNODPAIR)
