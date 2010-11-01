CC = gcc
CFLAGS = -Wall

all: nad_client nad_daemon

clean:
	rm -f *.o *~ .*~ nad_daemon nad_client

nad_client: commands.o nad_client.o

nad_daemon: commands.o nad_daemon.o