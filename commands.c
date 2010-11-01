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


#include <string.h>

char *commands[] =
  {
    "\rMain.Volume+\r", "\rMain.Volume-\r",

    "\rMain.Source+\r", "\rMain.Source-\r",
    "\rMain.Source=Aux\r", "\rMain.Source=Video\r",
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

int
command_list_len (void)
{
  int i;
  for (i = 0; commands[i] != NULL; i++);

  return i;
}
