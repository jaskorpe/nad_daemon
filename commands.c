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
