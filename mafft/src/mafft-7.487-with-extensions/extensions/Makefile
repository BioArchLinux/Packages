PREFIX = /usr/local
LIBDIR = $(PREFIX)/libexec/mafft
DESTDIR = 

#MNO_CYGWIN = -mno-cygwin

INSTALL = install

PROGS = mxscarnamod
SCARNASRC = mxscarna_src/* mxscarna_src/*/*

all : $(PROGS)
	cp $(PROGS) ../binaries
	chmod 755 ../binaries/*
	@echo done.

univscript: univscript.tmpl Makefile
	sed "s:_PROGS:$(PROGS):" univscript.tmpl  > univscript


mxscarnamod: $(SCARNASRC)
	$(MAKE) CFLAGS1="$(MNO_CYGWIN)" -C mxscarna_src
	mv mxscarna_src/mxscarna mxscarnamod


clean :
	rm -f *.o *.a *.exe *~ $(PROGS)
#	rm -f ../binaries/* ../scripts/*
	$(MAKE) clean -C mxscarna_src

install : all
	mkdir -p $(DESTDIR)$(LIBDIR)
	chmod 755 $(DESTDIR)$(LIBDIR)
	chmod 755 $(PROGS)
	$(INSTALL) $(PROGS) $(DESTDIR)$(LIBDIR)
