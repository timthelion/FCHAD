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
#include "io_generic.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

#define ERROR             0
#define CURSOR_DRIVER     1
#define BRLTTY_DRIVER     2
#define BUFFER_SIZE_MAX   3
#define BUFFER_COLUMNS    4
#define BUFFER_ROWS       5
#define DOTCOUNT          6
#define END_HEADER        7

#define NumberOfSettings  8

char * settings[]={
  "ERROR",
  "CURSOR_DRIVER",
  "BRLTTY_DRIVER",
  "BUFFER_SIZE_MAX",
  "BUFFER_COLUMNS",
  "BUFFER_ROWS",
  "DOTCOUNT",
  "END_HEADER"};


int  buffer_columns=0;
int  buffer_rows=0;

#define lineBufferLength 40 
char * brltty_driver="BRLTTY_C";
char cursor_driver[lineBufferLength];
unsigned char charicter;

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
static void Serial_print(char * line)
{
    printf(line);
}

static void Serial_println(char * line)
{
    Serial_print(line);
    Serial_print("\n");
}

static void Serial_read()
{
    //Read one byte from serial.
}

static void Serial_write()
{
    //Write one byte to serial.
}

static void Serial_nextLine(char * buffer)
{
    gets(buffer);
}

static void identify_mode_send_settings(){
  Serial_println("BRLTTY DRIVER");
  Serial_print(settings[BRLTTY_DRIVER]);
      Serial_print("=");
      Serial_println(brltty_driver);
  Serial_println(settings[END_HEADER]);
}

unsigned char identify_mode_setting_eq(char * setting, char * buffer){
  unsigned char i=0;
  while(1)
  {
    if(setting[i]==buffer[i]&&buffer[i]=='=')return i+1;
    if(setting[i]!=buffer[i])return 0;
    i++;
  }
}


unsigned char identify_mode_which_setting(char * buffer){
    unsigned char i;
    for(i=0;i<NumberOfSettings;i++){
        charicter=identify_mode_setting_eq(settings[i], buffer);
        if(charicter)
            return i;
    }
    return ERROR;
}

static void identify_mode_receive_settings(char * sender){
    char current_line[lineBufferLength];
    Serial_nextLine(current_line);
    if(0!=strcmp(current_line,sender))
    {
        Serial_print("Protocol error, expected:");
        Serial_print(sender);
        Serial_print(": got :");
        Serial_println(current_line);
    }
    unsigned char setting;
    while(1){
        Serial_nextLine(current_line);
        setting = identify_mode_which_setting(current_line);
        switch(setting){
            case CURSOR_DRIVER:  strcpy(&current_line[charicter],cursor_driver);
                                    break;
            case BRLTTY_DRIVER:  strcpy(&current_line[charicter],brltty_driver);
                                    break;
            case BUFFER_COLUMNS: buffer_columns=atoi(&current_line[charicter]);
                                    break;
            case BUFFER_ROWS:    buffer_rows=atoi(&current_line[charicter]);
                                    break;
            case END_HEADER:     return;break;
            case ERROR:         Serial_print("Unknown setting:");
                                Serial_println(itoa(current_line));break;
            default:;
        }
    }
}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  char current_line[lineBufferLength];
  Serial_nextLine(current_line);
  if(strcmp(current_line,"FCHAD")!=0)return 0;
  Serial_println("BRLTTY DRIVER - FCHAD?");
  identify_mode_receive_settings("FCHAD");
  identify_mode_send_settings();
  do{
    Serial_nextLine(current_line);
    current_line[4]="\0";//EVIL :D
  }while(strcmp(current_line,"WAIT")!=0);
//  delay(atoi(&current_line[5]));//EVIL :D :D
  identify_mode_receive_settings("FCHAD");
  return 1; //Go through initialization sequence with FCHAD device,
            //setting paramiters.
}

static void
brl_destruct (BrailleDisplay *brl) {
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
def brltty_fill_buffer(serial, buf,retry=False):
    if not retry: serialwrite(chr(2))
    #Tell the FCHAD device to enter read buffer mode.
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
    if not serialread(): brltty_fill_buffer(serial,buff,True)
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
