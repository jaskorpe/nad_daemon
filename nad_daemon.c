/* Copyright (C) 2010 Jon Anders Skorpen
 *
 * This file is part of nad_daemon.
 *
 * nad_daemon is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * nad_daemon is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with nad_daemon.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include <poll.h>

#define BUF_LEN 100
#define PORT "6789"

extern char *commands[];

int fd;
int sock;
int verbose = 0;

void
quit (char *message, int ret)
{
  if (verbose)
    printf("%s\n", message);
  close (fd);
  close (sock);
  exit (ret);
}

void
license (void)
{

}

int
validate_buf (char *buf, int recvlen)
{
  int ret = 0;
  int i;

  /* This will check that the received number of bytes is
   * as long as a command, and that the received command
   * is valid
   */

  for (i = 0; commands[i] != NULL; i++)
    {
      int s = strlen (commands[i]);

      if (s == recvlen && strncmp (buf, commands[i], s) == 0)
        {
          ret = 1;
          break;
        }
    }

  return ret;
}

int
tty_setup (char *filename)
{
  struct termios term;
  int fd;

  fd = open (filename, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
    {
      fprintf(stderr, "Unable to open %s\n", filename);
      return -1;
    }

  if (fcntl (fd, F_SETFL, 0) == -1)
    {
      perror ("Could not set file status flags");
      return -1;
    }


  /* Disable stop bit, parity bit and set word size.
   * Also set term speed, and disable all flow control
   */
  if (tcgetattr(fd, &term) == -1)
    {
      perror ("Could not get terminal attributes");
      return -1;
    }

  cfsetispeed(&term, B115200);
  cfsetospeed(&term, B115200);

  term.c_cflag |= CLOCAL | CREAD;
  term.c_cflag &= ~PARENB;
  term.c_cflag &= ~CSTOPB;
  term.c_cflag &= ~CSIZE;
  term.c_cflag |= CS8;

#ifdef CNEW_RTSCTS
  term.c_cflag &= ~CNEW_RTSCTS;
#endif

  if (tcsetattr (fd, TCSANOW, &term) == -1)
    {
      perror ("Could not update terminal attributes");
      return -1;
    }

  return fd;
}


static int
sock_bind (struct addrinfo *rp)
{
  int sock;
  int sock_opt = 1;

  sock = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);
  if (sock == -1)
    return -1;

  if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR,
                  &sock_opt, sizeof sock_opt)
      == -1)
    perror ("Could not set socket option");

  if (bind (sock, rp->ai_addr, rp->ai_addrlen) == 0)
    return sock;


  close (sock);
  return -1;
}


int
sock_setup (char *port, char *ip)
{
  int sock;
  int sock4 = -1;
  int sock6 = -1;

  int ret;

  struct addrinfo hints;
  struct addrinfo *result, *rp;

  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_canonname = NULL;
  hints.ai_next = NULL;

  ret = getaddrinfo (ip, port, &hints, &result);
  if (ret != 0)
    {
      fprintf (stderr, "Could not get host info: %s\n", gai_strerror (ret));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      if (rp->ai_family == AF_INET)
        {
          sock4 = sock_bind (rp);
        }
      else if (rp->ai_family == AF_INET6)
        {
          sock6 = sock_bind (rp);
        }
      else
        {
          fprintf (stderr, "Could not find any suitable interfaces\n");
          return -1;
        }
    }

  freeaddrinfo (result);

  if (sock6 != -1)
    {
      sock = sock6;
      close (sock4);
    }
  else if (sock4 != -1)
    {
      sock = sock4;
      close (sock6);
    }
  else
    {
      fprintf (stderr, "Could not bind to any socket\n");
      return -1;
    }


  if (listen (sock, 5) == -1)
    {
      perror ("Could not listen on socket");
      return -1;
    }

  return sock;
}

void
signal_handler (int signum)
{
  quit ("Caught signal\nClosing socket and tty\n", 0);
}

int
main (int argc, char **argv)
{
  int connected_sock;
  int ret;
  int c;
  int daemon_mode = 0;

  socklen_t addrlen;

  char buf[BUF_LEN];

  char *ip = NULL;
  char *port = PORT;

  char *filename = "/dev/ttyS0";

  struct sockaddr_storage connected_addr;

  struct pollfd fds;

  while ((c = getopt (argc, argv, "dvhp:a:t:")) != -1)
    {
      switch (c)
        {
        case 't':
          filename = optarg;
          break;
        case 'p':
          port = optarg;
          break;
        case 'a':
          ip = optarg;
          break;
        case 'v':
          verbose = 1;
          break;
        case 'd':
          daemon_mode = 1;
          break;
        case 'h':
          printf ("Usage: %s [-v] [-a <address>] [-p <port>] ", argv[0]);
          printf ("[-h] [-d] [-t tty]\n");

          printf (" -v\n\tVerbose output.\n");
          printf (" -d\n\tFork to background. ");
          printf ("After forking, output goes to /dev/null\n");

          printf (" -h\n\tPrint help.\n");
          printf (" -p port\n\tChoose which port to bind to.\n");
          printf (" -a address\n\tChoose which address to bind to.\n");
          printf (" -t tty\n\tChoose which tty to use.\n");
          exit (0);
        }
    }

  /* Setup of tty
   */
  if ((fd = tty_setup(filename)) == -1)
    return -1;
  if (verbose)
    printf ("%s open and ready for use\n", filename);

  /* Setup of socket
   */
  if ((sock = sock_setup (port, ip)) == -1)
    return -1;
  if (verbose)
    printf ("Socket listening on port: %s\n", port);

  signal (SIGINT, signal_handler);

  if (daemon_mode)
    {
      if (verbose)
        printf ("Forking to background...\n");
      if (daemon (0, 0) == -1)
        perror ("Could not fork to background");
    }


  while (1)
    {
      addrlen = sizeof connected_addr;
      if ((connected_sock =
           accept (sock, (struct sockaddr *)&connected_addr, &addrlen)) == -1)
        {
          perror ("Could not accept incoming connection");
          continue;
        }

      fds.fd = connected_sock;
      fds.events = 0 | POLLIN;
      fds.revents = 0;

      ret = poll (&fds, 1, 500);

      switch (ret)
        {
        case -1:
        case 0:
          /* Either some error or timeout.
           * continue statement apparantly works on
           * the while loop and not on the swithc
           */
          if (send (connected_sock, "Timeout", 8, 0) == -1)
            perror ("Could not send timeout message");

          close (connected_sock);
          continue;
        }

      ret = recv (connected_sock, buf, BUF_LEN, 0);

      if (!validate_buf (buf, ret))
        {
          if (send (connected_sock, "Invalid message", 16, 0) == -1)
            perror ("Could not send invalid message message");

          close (connected_sock);
          continue;
        }

      if (verbose)
        {
          printf ("Command: ");
          fwrite (buf+1, ret-2, 1, stdout);
          printf ("\n");
        }

      if ((ret = write (fd, buf, ret)) == -1)
        {
          perror ("Problem writing to device\n");
          quit ("Exiting...", -1);
          return -1;
        }

      if (verbose)
        printf ("Bytes written: %d\n", ret);

      if (send (connected_sock, "Success", 8, 0) == -1)
        perror ("Could not send success message");
      close (connected_sock);

    }

  if (verbose)
    quit ("Exiting...", 0);

  return 0;
}
