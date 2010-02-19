#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

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
    "\rMain.Volume+\r", "\rMain.Volume-\r", "\rMain.Source+\r",
    "\rMain.Source-\r", "\rMain.Mute+\r", "\rMain.Mute-\r",
    "\rMain.Mute=On\r", "\rMain.Mute=Off\r", "\rMain.Power+\r",
    "\rMain.Power-\r", "\rMain.Power=On\r", "\rMain.Power=Off\r",
    "\rMain.Source=Aux\r", "\rMain.Source=Video1\r", "\rMain.Source=CD\r",
    "\rMain.Source=Tuner\r", "\rMain.Source=Disc\r", NULL
  };

int
validate (char *buf)
{
  int ret = 0;
  int i;

  for (i = 0; buf[i] != NULL; i++)
    if (!strcmp (buf, commands[i]))
      {
	ret = 1;
	break;
      }

  return ret;
}

int
main (int argc, char **argv)
{
  int fd;
  int sock;
  int connected_sock;
  int ret;
  int addrlen;

  char *filename = "/dev/ttyS0";
  char buf[BUF_LEN];

  struct termios term;

  struct sockaddr_in addr;
  struct sockaddr_in connected_addr;

  struct pollfd fds;

  if (argc == 1)
    {
      printf ("USAGE: %s [<file>]", argv[0]);
      return -1;
    }
  filename = argv[1];

  fd = open (filename, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
    {
      printf("Unable to open %s\n", filename);
      return -1;
    }

  fcntl (fd, F_SETFL, 0);


  /* Disable stop bit, parity bit and set word size.
   * Also set term speed, and disable all flow control
   */
  tcgetattr(fd, &term);

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

  tcsetattr (fd, TCSANOW, &term);


  /* Set up of socket
   */
  if ((sock = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
      perror ("Could not create socket");
      return -1;
    }

  memset (&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (PORT);
  addr.sin_addr.s_addr = INADDR_ANY;

  if (bind (sock, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1)
    {
      perror ("Could not bind socket to address");
      return -1;
    }

  if (listen (sock, 5) == -1)
    {
      perror ("Could not listen on socket");
      return -1;
    }

  while (1)
    {
      printf ("KVA?\n");
      addrlen = sizeof(struct sockaddr_in);
      if ((connected_sock = accept (sock, (struct sockaddr *)&connected_addr, &addrlen) == -1))
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
	  if ((ret = send (connected_sock, "Timeout", 8, 0)) == -1)
	    perror ("Could not send timeout message");

	  close (connected_sock);
	  continue;
	}

      ret = recv (connected_sock, buf, BUF_LEN, 0);

      if (!validate (buf))
	{
	  send (connected_sock, "Invalid message", 16, 0);
	  close (connected_sock);
	  continue;
	}

      printf ("buf: %s\n", buf);

      ret = write (fd, buf, strlen(buf));
      
      if (ret == -1)
	{
	  perror ("Problem writing to device\n");
	  close (fd);
	  return -1;
	}

      printf ("Bytes written: %d\n", ret);

      send (connected_sock, "Success", 8, 0);
      close (connected_sock);

    }

  close (fd);
  close (sock);
}
