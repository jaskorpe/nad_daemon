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
#define PORT 6789


char *commands[] =
  {
    "\rMain.Volume+\r", "\rMain.Volume-\r",

    "\rMain.Source+\r", "\rMain.Source-\r",
    "\rMain.Source=Aux\r", "\rMain.Source=Video1\r",
    "\rMain.Source=CD\r", "\rMain.Source=Tuner\r",
    "\rMain.Source=Disc\r",

    "\rMain.Mute+\r", "\rMain.Mute-\r",
    "\rMain.Mute=On\r", "\rMain.Mute=Off\r",

    "\rMain.Power+\r", "\rMain.Power-\r",
    "\rMain.Power=On\r", "\rMain.Power=Off\r",

    "\rMain.SpeakerA+\r", "\rMain.SpeakerA-\r",
    "\rMain.SpeakerB+\r", "\rMain.SpeakerB-\r",
    "\rMain.SpeakerA=On\r", "\rMain.SpeakerA=Off\r",
    "\rMain.SpeakerB=On\r", "\rMain.SpeakerB=Off\r",
    NULL
  };

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

int
validate_buf (char *buf)
{
  int ret = 0;
  int i;

  for (i = 0; commands[i] != NULL; i++)
    if (strcmp (buf, commands[i]) == 0)
      {
	ret = 1;
	break;
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
      printf("Unable to open %s\n", filename);
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

int
sock_setup (int port, char *ip)
{
  int sock;
  struct sockaddr_in addr;
  int sock_opt = 1;

  memset (&addr, 0, sizeof addr);
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);

  if (ip)
    addr.sin_addr.s_addr = inet_addr (ip);
  else
    addr.sin_addr.s_addr = INADDR_ANY;

  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Could not create socket");
      return -1;
    }

  if (setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof sock_opt) == -1)
    perror ("Could not set socket option");

  if (bind (sock, (struct sockaddr *)&addr, sizeof addr) == -1)
    {
      perror ("Could not bind socket to address");
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
  int port = PORT;
  int c;
  int daemon_mode = 0;

  socklen_t addrlen;

  char buf[BUF_LEN];
  char *ip = NULL;
  char *filename = "/dev/ttyS0";

  struct sockaddr_in connected_addr;

  struct pollfd fds;

  while ((c = getopt (argc, argv, "dvhp:a:t:")) != -1)
    {
      switch (c)
	{
	case 't':
	  filename = optarg;
	  break;
	case 'p':
	  port = atoi (optarg);
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
	  printf ("Usage: %s [-v] [-a <address>] [-p <port>] [-h] [-d] [-t tty]\n", argv[0]);
	  printf (" -v\n\tVerbose output.\n");
	  printf (" -d\n\tFork to background. After forking, output goes to /dev/null\n");
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
    printf ("Socket listening on port: %d\n", port);

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
      if ((connected_sock = accept (sock, (struct sockaddr *)&connected_addr, &addrlen)) == -1)
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

      if (!validate_buf (buf))
	{
	  if (send (connected_sock, "Invalid message", 16, 0) == -1)
	    perror ("Could not send invalid message message");

	  close (connected_sock);
	  continue;
	}

      if (verbose)
	printf ("buf: %s\n", buf);

      if ((ret = write (fd, buf, strlen(buf))) == -1)
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
