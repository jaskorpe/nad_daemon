CC = gcc
CFLAGS = -Wall

PREFIX=/usr/local
INSTALL_DIR=$(DESTDIR)$(PREFIX)/bin


.PHONY: debug all clean install

all: nad_client nad_daemon

debug:CFLAGS+=-g
debug:nad_client nad_daemon

clean:
	rm -f *.o *~ .*~ nad_daemon nad_client

nad_client: commands.o nad_client.o

nad_daemon: commands.o nad_daemon.o

install: install_client install_daemon

install_client: nad_client
	install -s -v -D $< $(INSTALL_DIR)/$<

install_daemon: nad_daemon
	install -s -v -D $< $(INSTALL_DIR)/$<