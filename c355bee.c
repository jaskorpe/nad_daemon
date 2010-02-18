#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

int
main(int argc, char **argv)
{
  int fd;
  int ret = 0;

  char *filename = "/dev/ttyS0";
  char *value = "?";
  char *command = "Model";
  char buf[100];

  struct termios term;

  switch (argc)
    {
    case 4: filename = argv[3];
    case 3: value = argv[2];
    case 2: command = argv[1]; break;
    case 1:
    default:
      printf("USAGE: %s <command> [<value>] [<filename>]\n",
	     argv[0]);
      return -1;
    }

  /* Important to do proper checking of input here! */


  fd = open (filename, O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1)
    {
      printf("Unable to open %s\n", filename);
      return -1;
    }

  fcntl(fd, F_SETFL, 0);

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


  memcpy (buf, "\rMain.", 6);
  memcpy (buf+6, command, strlen(command));
  memcpy (buf+6+strlen(command), value, strlen(value));
  memcpy (buf+6+strlen(command)+strlen(value), "\r\0", 2);

  printf("Bytes to write: %d\nString to write %s\nDevice: %s\n",
	 strlen(buf), buf, filename);

  ret = write (fd, buf, strlen(buf));

  if (ret == -1)
    {
      perror ("Problem writing to device\n");
      close(fd);
      return -1;
    }

  printf("Bytes written: %d\n", ret);

  //ret = read(fd, buf, 100);
  //printf("Response: %s\nBytes: %d\n", buf, buf[0]);

  close(fd);

  return (fd);
}
