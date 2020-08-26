CXX = g++ $(STD)

STD = -std=gnu++20
CXXFLAGS = $(DEFINES) $(INCLUDES) $(OPT) $(DEBUG) $(WARN)

DEFINES =
INCLUDE = $(shell $(PKG_CONFIG) --cflags avahi-client)
OPT = -O0
DEBUG = -g3
WARN = -Wall

LIBOBJS = keylightpp.o

LIBS = $(shell $(PKG_CONFIG) --libs avahi-client) -lcpprest -lcrypto -lpthread

PKG_CONFIG = pkg-config

all: keylight

keylight: cli.o libkeylightpp.a
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

libkeylightpp.a: $(LIBOBJS)
	$(AR) rcs $@ $^

cli.o keylightpp.o: keylightpp.hh

clean:
	rm -f keylight cli.o libkeylightpp.a $(LIBOBJS)

.PHONY: all clean
