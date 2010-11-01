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

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string.h>
#include <getopt.h>


#define BUF_LEN 100

extern char *commands[];
int command_list_len (void);

void
usage (char *name)
{
  printf ("Usage: %s [-6] [-4] [-h] <host> <port> <command>\n", name);

  printf (" -h Print help.\n");
  printf (" -6 Use IPv6.\n");
  printf (" -4 Use IPv4.\n");
  printf (" -l List commands.\n");
  exit (0);
}


void
list (void)
{
  int i;
  int len = command_list_len ();

  printf ("Supported commands: (%d)\n", len);

  for (i = 0; i < len; i++)
    printf ("%d - %s\n", i, commands[i]+1);

  exit (0);
}


int
main (int argc, char **argv)
{
  struct addrinfo hints;
  struct addrinfo *result, *rp;

  int sock;
  int prot = 0;
  int c;
  int ret;

  char buf[BUF_LEN];
  char *host;
  char *port;
  int command;

  while ((c = getopt (argc, argv, "l6h4")) != -1)
    {
      switch (c)
        {
        case '6':
          prot = 6;
          break;
        case '4':
          prot = 4;
          break;
        case 'l':
          list ();
          break;
        case 'h':
        default:
          usage (argv[0]);
        }
    }

  if (optind < argc)
    host = argv[optind++];
  else
    usage (argv[0]);

  if (optind < argc)
    port = argv[optind++];
  else
    usage (argv[0]);

  if (optind < argc)
    command = atoi (argv[optind++]);
  else
    usage (argv[0]);


  memset (&hints, 0, sizeof (struct addrinfo));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_flags = 0;

  if (prot == 6)
    hints.ai_family = AF_INET6;
  else if (prot == 4)
    hints.ai_family = AF_INET;
  else
    hints.ai_family = AF_UNSPEC;

  ret = getaddrinfo (host, port, &hints, &result);
  if (ret != 0)
    {
      fprintf (stderr, "Could not get host info: %s\n", gai_strerror (ret));
      return -1;
    }

  for (rp = result; rp != NULL; rp = rp->ai_next)
    {
      sock = socket (rp->ai_family, rp->ai_socktype, rp->ai_protocol);

      if (sock == -1)
        continue;

      if (connect (sock, rp->ai_addr, rp->ai_addrlen) != -1)
        break;

      close (sock);
    }

  if (rp == NULL)
    {
      fprintf (stderr, "Could not connect to host: %s, at port: %s\n",
               host, port);
      return -1;
    }



  if (command >= command_list_len () || command < 0)
    {
      printf ("Invalid command\n");
      close (sock);
      return -1;
    }

  freeaddrinfo (result);

  write (sock, commands[command], strlen (commands[command]));

  read (sock, buf, BUF_LEN);

  printf ("%s", buf);

  close (sock);

  return 0;
}
