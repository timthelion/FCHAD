/*
 * BRLTTY - A background process providing access to the console screen (when in
 *          text mode) for a blind person using a refreshable braille display.
 *
 * Copyright (C) 2012 by Timothy Hobbs and
 * Copyright (C) 1995-2011 by The BRLTTY Developers.
 *
 * BRLTTY and the FCHAD software comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://mielke.cc/brltty/ and  http://brmlab.cz/user/timthelion
 *
 * This software is maintained by Timothy Hobbs <timothyhobbs@seznam.cz>.
 */

#include "prologue.h"

#include <stdio.h>

//typedef enum {
//  PARM_XXX,
//  ...
//} DriverParameter;
//#define BRLPARMS "xxx", ...

//#define BRL_STATUS_FIELDS sf...
//#define BRL_HAVE_STATUS_CELLS
//#define BRL_HAVE_PACKET_IO
//#define BRL_HAVE_KEY_CODES
#include "brl_driver.h"

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  return 0; //Go through initialization sequence with FCHAD device,
            //setting paramiters.
}

static void
brl_destruct (BrailleDisplay *brl) {
}

#ifdef BRL_HAVE_PACKET_IO
static ssize_t
brl_readPacket (BrailleDisplay *brl, unsigned char *buffer, size_t size) {
  return -1;
}

static ssize_t
brl_writePacket (BrailleDisplay *brl, const unsigned char *packet, size_t length) {
  return -1;
}

static int
brl_reset (BrailleDisplay *brl) {
  return 0;
}
#endif /* BRL_HAVE_PACKET_IO */

#ifdef BRL_HAVE_KEY_CODES
static int
brl_readKey (BrailleDisplay *brl) {
  return EOF;
}

static int
brl_keyToCommand (BrailleDisplay *brl, KeyTableCommandContext context, int key) {
  return EOF;
}
#endif /* BRL_HAVE_KEY_CODES */

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  return 1;//Fill the FCHAD's buffer.  Are the bytes in the brl->buffer ASCII
  //chars, or "braille bytes" with one bit acounting for each dot?  If the latter
  //(preferable) then is it possible to request 6 dot braille from brltty?  The device
  //driver should support both 6 and 8 dot braille(my current devices use 6 dots and
  //for my purposes 6 dots may be prefered.)
}

#ifdef BRL_HAVE_STATUS_CELLS
static int
brl_writeStatus (BrailleDisplay *brl, const unsigned char *status) {
  return 1;
}
#endif /* BRL_HAVE_STATUS_CELLS */

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;//What is the difference between brl_readCommand and brl_readKey
             //Finish initialization sequence after cursor driver is regisered
             //with the FCHAD device.  Then go into idle mode.
             //When in idle mode, we read two byte sequences sent by the FCHAD
             //device.  These are key codes.
}
