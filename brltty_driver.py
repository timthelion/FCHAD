#define PYTHON 0

#if PYTHON
"""
#endif

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

#if PYTHON
"""

from serial import Serial
import sys, os, time, math

keycodes = {
    1: lambda : "Previous line.",
    2: lambda : "Next line.",
    3: lambda : "Braille cell pressed at possition:"
    }

settings={
    "BRLTTY_DRIVER" : [lambda raw_setting: raw_setting , "Dummy python"],
    "CURSOR_DRIVER" : [lambda raw_setting: raw_setting ,""],
    "BUFFER_SIZE_MAX" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_COLUMNS" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_ROWS" : [lambda raw_setting: int(raw_setting), 0],
    "DOTCOUNT" : [lambda raw_setting: int(raw_setting), 0]
}

settings_to_send={"BRLTTY_DRIVER"}#which of the settings in settings(by key),
#should we send to the FCHAD?

#endif

#if PYTHON
"""
#endif

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  return 0; //Go through initialization sequence with FCHAD device,
            //setting paramiters.
}

#if PYTHON
"""
def brltty_init(serial):
    DEVICE_ID = serialreadline()
    print "Device ID:" + DEVICE_ID
    if not DEVICE_ID.startswith("FCHAD"):
        print "Not an FCHAD device."
        sys.exit()
    serialwrite("BRLTTY DRIVER - FCHAD?\n")
    readSettings(serial)
    writeSettings(serial)
    print "Successfully initialized(we hope), now go start up the cursor driver."
    while not serial.inWaiting():1#wait for cursor driver to load.
    wait_command=serialreadline()
    pwait_command=wait_command.partition(" ")
    if pwait_command[0]=="WAIT":
        time.sleep(1000.0/int(pwait_command[2]))
        readSettings(serial)
    else:
        print "PROTOCOL ERROR, I EXPECTED 'WAIT' INSTEAD I GOT:"+wait_command
        sys.exit()
    readSettings(serial)


def readSettings(serial):
    psettings={}
    setting=serialreadline()
    setting=setting.partition("\r")[0]#stupid pyserial gets not only the line, 
                                      #but the line return as well...
    if not setting == "FCHAD":
        print "Protocol error, expected FCHAD, got:"+setting
    while 1:
        setting=serialreadline()
        setting=setting.partition("\r")[0]#stupid pyserial gets not only the line, 
                                          #but the line return as well...
        if setting == "END_HEADER": break
        psetting=setting.partition("=")
        if settings[psetting[0]][0](psetting[2]):
            try:
                settings[psetting[0]][1]=settings[psetting[0]][0](psetting[2])
            except KeyError:
                print "Unknown setting:"+setting
        
def writeSettings(serial):
    serialwrite("BRLTTY DRIVER\n")
    for setting in settings_to_send:
        serialwrite(setting+"="+str(settings[setting][1])+"\n")
    serialwrite("END_HEADER\n")

#endif

#if PYTHON
"""
#endif

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
  return 1;//Fill the FCHAD's buffer.  Are the bytes in the brl->buffer ASCII
  //chars, or "braille bytes" with one bit acounting for each dot?  If the latter
  //(preferable) then is it possible to request 6 dot braille from brltty?  The device
  //driver should support both 6 and 8 dot braille(my current devices use 6 dots and
  //for my purposes 6 dots may be prefered.)
}

#if PYTHON
"""
def brltty_fill_buffer(serial, buf):
    serialwrite(chr(2))#Tell the FCHAD device to enter read buffer mode.
    checksum = 0
    for braille_byte in buff:#Write to the buffer.
        if braille_byte==0: serialwrite(chr(braille_byte))
        #keeping in mind that 0 is an escape charicter and must be written 
        #twice.  This is for a single row buffer support exists for more rows.
        #To go to the next row, use the escape sequence 00 01.  The C driver
        #should support multi row buffers, but since the current cursor driver
        #does not take advantage of this capability, I won't implement it now.
        serialwrite(chr(braille_byte))
        checksum = checksum + braille_byte
    serialwrite(chr(0))#Close read/write buffer mode.
    serialwrite(chr(2))
    #write check sum
    checksum_bytes=[]
    checksum_bytes.append(chr((checksum&0xFF000000)>>24))
    checksum_bytes.append(chr((checksum&0x00FF0000)>>16))
    checksum_bytes.append(chr((checksum&0x0000FF00)>>8))
    checksum_bytes.append(chr( checksum&0x000000FF))
    for checksum_byte in checksum_bytes:
        serialwrite(checksum_byte)
#endif

#if PYTHON
"""
#endif

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
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;//What is the difference between brl_readCommand and brl_readKey
             //Finish initialization sequence after cursor driver is regisered
             //with the FCHAD device.  Then go into idle mode.
             //When in idle mode, we read two byte sequences sent by the FCHAD
             //device.  These are key codes.
}

#if PYTHON
"""
def getKeyCode(serial):
    while not serial.inWaiting()>2:1
    keycode = ord(serialread()) << 8
    return keycode + ord(serialread())


def readNextKey(serial):
    keycode=getKeyCode(serial)
    key = keycodes[keycode]()
    print keycode
    print key
    if keycode == 3:
        x=getKeyCode(serial)
        y=getKeyCode(serial)
        print str(x) + "," + str(y)
###################################################
###Evil nasty logging serial functions#############
###################################################
def serialread():        
    x=serial.read()
    if serialLogFile:
        serialLogFile.write("##READ##")
        serialLogFile.write(x)
    return x

def serialreadline():
    x=serial.readline()
    if serialLogFile:
        serialLogFile.write("##READ_LINE##")
        serialLogFile.write(x)
    return x

def serialwrite(x):
    serial.write(x)
    if serialLogFile:
        serialLogFile.write("##WRITE##")
        serialLogFile.write(x)
    

        
###################################################
###################################################

serial = Serial(sys.argv[1], 9600, timeout=1)
help = "--help" in sys.argv or "-h" in sys.argv
log = "--log" in sys.argv
serialLogFile=None

if help:
    m = """BRLTTY driver for controlling FCHAD type devices. Start this first
    then the cursor driver.  Then orca...  This driver is a dummy or prototype,
    if you actually want to use brltty, use the C version...
    
    First argument passed should be path of USB serial FCHAD device.
    Aka /dev/ttyUSB0.
    
    --log Log contents of serial transactions to file.
    
    -h,--help Display this message.
    """
    print m
else:
    if log:
        serialLogFile = open('serialLogFile.log', 'w')
        serialLogFile.write("Beginning of log file:\n")
    brltty_init(serial)
    print "Filling buffer."
    buff=[]
    for i in range(settings["BUFFER_COLUMNS"][1]):
                                               #this is for a single row buffer
                                               #support exists for more rows.
        if i > 255: i=i-255*math.floor(i/255)
        buff.append(i)
    brltty_fill_buffer(serial, buff)
    print "Buffer filled."
    print "Reading key codes from serial."
    while True:readNextKey(serial)

#endif
