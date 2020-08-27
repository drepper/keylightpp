VERSION = 0.1
ABI = 1

CXX = g++ $(CXXSTD)
GAWK = gawk
INSTALL = install
CAT = cat
LN_FS = ln -fs
SED = sed
TAR = tar
MV_F = mv -f
RM_F = rm -f
RPMBUILD = rpmbuild
PKG_CONFIG = pkg-config

CXXSTD = -std=gnu++20
CXXFLAGS = $(OPT) $(DEBUG) $(CXXFLAGS-$@) $(WARN)
CPPFLAGS = $(INCLUDES) $(DEFINES)
LDFLAGS = $(LDFLAGS-$@)

DEFINES =
INCLUDES = $(shell $(PKG_CONFIG) --cflags $(DEPPKGS))
OPT = -O0
DEBUG = -g3
WARN = -Wall

LIBS = -lcpprest $(shell $(PKG_CONFIG) --libs $(DEPPKGS)) -lpthread

prefix = /usr
includedir = $(prefix)/include/keylightpp-$(VERSION)
libdir = $(prefix)/$(shell $(CXX) -print-file-name=libc.so | $(GAWK) -v FS='/' '{ print($$(NF-1)) }')
pcdir= $(libdir)/pkgconfig

IFACEPKGS = 
DEPPKGS = avahi-client libcrypto
ALLPKGS = $(IFACEPKGS) $(DEPPKGS)


all: keylight libkeylightpp.a libkeylightpp.so

.cc.os:
	$(COMPILE.cc) -fpic -o $@ $<

keylight: cli.o libkeylightpp.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

libkeylightpp.a: keylightpp.o
	$(AR) rcs $@ $^

libkeylightpp.so: keylightpp.os
	$(LINK.cc) -shared -Wl,-h,libkeylightpp.so.$(ABI) -o $@ $^ $(LIBS)

cli.o keylightpp.o keylightpp.os: keylightpp.hh

keylightpp.pc: Makefile
	$(CAT) > $@-tmp <<EOF
	prefix=$(prefix)
	includedir=$(includedir)
	libdir=$(libdir)

	Name: KeyLight++
	Decription: C++ Library to interace with KeyLight devices
	Version: $(VERSION)
	URL: https://github.com/drepper/keylightpp
	Requires: $(IFACEPKGS)
	Requires.private: $(DEPPKGS)
	Cflags: -I$(includedir)
	Libs: -L$(libdir) -lkeylightpp
	EOF
	$(MV_F) $@-tmp $@

keylightpp.spec: keylightpp.spec.in Makefile
	$(SED) 's/@VERSION@/$(VERSION)/;s/@ABI@/$(ABI)/' $< > $@-tmp
	$(MV_F) $@-tmp $@

install: libkeylightpp.a keylightpp.pc
	$(INSTALL) -D -c -m 755 libkeylightpp.so $(DESTDIR)$(libdir)/libkeylightpp-$(VERSION).so
	$(LN_FS) libkeylightpp-$(VERSION).so $(DESTDIR)$(libdir)/libkeylightpp.so.$(ABI)
	$(LN_FS) libkeylightpp.so.$(ABI) $(DESTDIR)$(libdir)/libkeylightpp.so
	$(INSTALL) -D -c -m 644 libkeylightpp.a $(DESTDIR)$(libdir)/libkeylightpp.a
	$(INSTALL) -D -c -m 644 keylightpp.pc $(DESTDIR)$(pcdir)/keylightpp.pc
	$(INSTALL) -D -c -m 644 keylightpp.hh $(DESTDIR)$(includedir)/keylightpp.hh

dist: keylightpp.spec
	$(LN_FS) . keylightpp-$(VERSION)
	$(TAR) achf keylightpp-$(VERSION).tar.xz keylightpp-$(VERSION)/{Makefile,keylightpp.hh,keylightpp.cc,cli.cc,README.md,keylightpp.spec,keylightpp.spec.in}
	$(RM_F) keylightpp-$(VERSION)

srpm: dist
	$(RPMBUILD) -ts keylightpp-$(VERSION).tar.xz
rpm: dist
	$(RPMBUILD) -tb keylightpp-$(VERSION).tar.xz

clean:
	rm -f keylight cli.o libkeylightpp.a libkeylightpp.so keylightpp.o keylightpp.os \
	      keylightpp.spec keylightpp.pc

.PHONY: all install dist srpm rpm clean
.SUFFIXES: .os
.ONESHELL:
