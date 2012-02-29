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


int  buffer_columns=20;//Defaults make testing easier.
                       //Are never used in real life.
int  buffer_rows=1;
int buffer_size_max=800;

#define lineBufferLength 40 
char * brltty_driver="BRLTTY_C";
char cursor_driver[lineBufferLength];
unsigned char charicter;

static unsigned char *previousCells = NULL; /* previous pattern displayed */

#define SERIAL 1
#define SERIAL_BAUD 9600
#define SERIAL_READY_DELAY 400
#define SERIAL_INPUT_TIMEOUT 100
#define SERIAL_WAIT_TIMEOUT 200
static GioEndpoint *gioEndpoint = NULL;

/*
"""
from serial import Serial
import sys, os, time, math
from FCHADsettings import FCHAD_setting_manager
from loggingSerial import *

settings_to_send=["BRLTTY_DRIVER"]#which of the settings in settings(by key),
#should we send to the FCHAD?

keycodes = {
    1: lambda : "Previous line.",
    2: lambda : "Next line.",
    3: lambda : "Braille cell pressed at possition:"
    }

"""
*/

static void Serial_init(const char *identifier)
{//COPY PASTE from connectResource() in the Voyager driver...
    GioDescriptor descriptor;
    gioInitializeDescriptor(&descriptor);

    SerialParameters serialParameters;
    gioInitializeSerialParameters(&serialParameters);
    serialParameters.baud = SERIAL_BAUD;
    descriptor.serial.parameters = &serialParameters;

    descriptor.serial.options.readyDelay = SERIAL_READY_DELAY;
    descriptor.serial.options.inputTimeout = SERIAL_INPUT_TIMEOUT;

    if ((gioEndpoint = gioConnectResource(identifier, &descriptor))) {
      return 1;
    }
    return 0;
}

static void printByte(unsigned char byte)
{
    printf("CHAR:%c\n",byte);
    printf("DEC:%d\n",byte);    
    if(byte & (1))    printf("1"  ); else printf ("0"  );
    if(byte & (1<<3)) printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<1)) printf("1"  ); else printf ("0"  );
    if(byte & (1<<4)) printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<2))  printf("1"  ); else printf ("0"  );
    if(byte & (1<<5))  printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<6))  printf("1"  ); else printf ("0"  );
    if(byte & (1<<7))  printf("1\n"); else printf ("0\n");

}

static void Serial_print(char * line)
{
    #if SERIAL
    int size = strlen(line);
    gioWriteData(gioEndpoint, line, size);
    #else
    printf(line);
    #endif
}

static void Serial_println(char * line)
{
    Serial_print(line);
    Serial_print("\n");
}

static unsigned char Serial_read()
{
    #if SERIAL
    while(!gioAwaitInput(gioEndpoint,0));
    unsigned char byte;
    gioReadByte(gioEndpoint, &byte,0);
    printf("<<\n");
    printByte(byte);
    return byte;
    #else
    char * num[4];    
    gets(num);
    return atoi(num);
    #endif
    //Read one byte from serial.
}

static void Serial_write(unsigned char byte)
{
    //#if SERIAL
    gioWriteData(gioEndpoint, &byte, 0);
    //#else
    //printf(">>\n");
    //printByte(byte);
    //#endif
    //Write one byte to serial.
}

static void Serial_nextLine(char * buffer)
{
    #if SERIAL
    unsigned char byte=0;
    int i=0;
    while(byte!='\r'&&byte!='\n'&&i<lineBufferLength)
    {
        do{
            while(!gioAwaitInput(gioEndpoint,0));
            gioReadByte(gioEndpoint, &byte,0);
        }while(byte=='\0');        
        buffer[i]=byte;
        i++;
    }
    buffer[i-1]='\0';
    if(i<=1)Serial_nextLine(buffer);
    printf(buffer);
    printf("\n");
    #else
    gets(buffer);
    #endif
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
    if(setting[i]=='\0')return i+1;
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

static int identify_mode_receive_settings(char * sender){
    char current_line[lineBufferLength];
    Serial_nextLine(current_line);
    while(0!=strcmp(current_line,sender))
    {
        Serial_nextLine(current_line);
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
            case BUFFER_SIZE_MAX: buffer_size_max=atoi(&current_line[charicter]);
                                    break;
            case END_HEADER:     return 1;break;
            case ERROR:         Serial_print("Unknown setting:");
                                Serial_println(current_line);
                                snprintf(current_line, lineBufferLength,"%d",charicter);
                                Serial_println(current_line);break;
            default:;
        }
    }
    return 0;//This should never happen.
}

static int init(BrailleDisplay *brl)
{
  char current_line[lineBufferLength];
  Serial_nextLine(current_line);
  if(strcmp(current_line,"FCHAD")!=0)
  {
     printf("Looking for an FCHAD device.  Got:\n");
     printf(current_line);
     printf(" instead.\n");
     printf("Try pressing the reset button on your device.\n");
     return 0; 
  }else
    printf("\nFCHAD device found.\n");
  Serial_println("BRLTTY DRIVER - FCHAD?");
  if(!identify_mode_receive_settings("FCHAD"))return 0;
  printf("Done getting settings from FCHAD. Sending brltty driver settings.\n");
  identify_mode_send_settings();
  printf("Settings sent, waiting for cursor driver to start up.\n");
  do{
    Serial_nextLine(current_line);
    current_line[4]='\0';//EVIL :D
  }while(strcmp(current_line,"WAIT")!=0);
  printf("Cusor driver loading...\n");
  approximateDelay(atoi(&current_line[5]));//EVIL :D :D
  printf("Updating settings...\n");
  Serial_println("BRLTTY DRIVER - FCHAD?");
  if(!identify_mode_receive_settings("FCHAD"))return 0;
  brl->textColumns=buffer_columns;
  brl->textRows=buffer_rows;
  previousCells = malloc(buffer_size_max);
  printf("Done with init.\n");
  printf("buffer_columns:%d\n",brl->textColumns);
  printf("buffer_rows:%d\n",brl->textRows);
  approximateDelay(1000);  
  return 1; //Go through initialization sequence with FCHAD device,
            //setting paramiters.

}

static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  Serial_init(device);
  while(!init(brl))approximateDelay(1000);
}

static void
brl_destruct (BrailleDisplay *brl) {
    //There is no destruct
}

/*
"""
def waitFor(command):
    while not serial.inWaiting():1#wait for cursor driver to load.
    #If we get something else, we return what we got.
    received_command=serialLogger.readline()
    if received_command==command:
        return None
    else:
        return received_command

def brltty_init():
    DEVICE_ID = serialLogger.readline()
    print "Device ID:" + DEVICE_ID
    if not DEVICE_ID.startswith("FCHAD"):
        print "Not an FCHAD device."
        sys.exit()
    print "Gettings settings from FCHAD."
    setting_manager.readSettings("BRLTTY DRIVER - FCHAD?\n")
    print "Sending settings to FCHAD."
    setting_manager.settings["BRLTTY_DRIVER"][1]="DummyPython"
    setting_manager.writeSettings("BRLTTY DRIVER\n",settings_to_send)
    print "Successfully initialized(we hope), now go start up the cursor driver."
    
    wait_command = waitFor("WAIT")
    if not wait_command:
        time.sleep(setting_manager.settings["SERIAL_WAIT_TIME"][1]/100.0)
        print "Reading settings..."
        setting_manager.readSettings("BRLTTY DRIVER - FCHAD?\n")
    else:
        print "PROTOCOL ERROR, I EXPECTED 'WAIT' INSTEAD I GOT:"+wait_command
        sys.exit()
    print "Updated settings..."

#endif


"""
*/
static char inbuffer(int x, int y, int columns, int rows)
{
    return x>=0 && y>=0 && x<columns && y<rows;
}

static void writeCheckSum(long checksum)
{
    Serial_write((uint8_t)((checksum&0xFF000000)>>24));
    Serial_write((uint8_t)((checksum&0x00FF0000)>>16));
    Serial_write((uint8_t)((checksum&0x0000FF00)>>8));
    Serial_write((uint8_t) (checksum&0x000000FF));
}

long readLong(){
  unsigned char cb1 = 10;
  while(cb1!=0)cb1=Serial_read();
  cb1=Serial_read();
  unsigned char cb2 = Serial_read();
  unsigned char cb3 = Serial_read();
  unsigned char cb4 = Serial_read();
  printByte(cb1);
  printByte(cb2);
  printByte(cb3);
  printByte(cb4);    
  long b1 = (uint32_t)cb1<<24;
  long b2 = (uint32_t)cb2<<16;
  long b3 = (uint32_t)cb3<<8;
  long b4 = (uint32_t)cb4;
  return b1+b2+b3+b4;
}


static int writeWindow(BrailleDisplay *brl, const wchar_t *text)
{
  printf("Entering write buffer mode.\n");
  if(!Serial_read()){
      printf("Something happended.");
      return 0;
  }
  printf("Writting buffer.\n");
  long checksum=0;
  long checksum_fchad;
  int buff_pos=0;
  int x_pos=0;
  int y_pos=0;
  while(1)
  {
      if(!inbuffer(x_pos,y_pos,brl->textColumns,brl->textRows))
      {
          printf("x:%d,y:%d,brl->textColumns:%d,brl->textRows:%d\n",x_pos,y_pos,brl->textColumns,brl->textRows);
          x_pos=0;
          y_pos++;
          if(y_pos>=brl->textRows)
          {
              printf("EOB\n");
              Serial_write(0);//End of buffer
              Serial_write(2);
              printf("Writting checksum:%d\n",checksum);
              writeCheckSum(checksum);
              checksum_fchad=readLong();
              printf("Checksum read:%d\n",checksum_fchad);
              if(checksum_fchad==checksum)
              {
                printf("Buffer written.\n");
                return 1;
              }
              else
              {
                printf("Checksum failed.\n");  
                return 0;//writeWindow(brl,text);
              }
          }else
          {
              printf("EOL\n");
              Serial_write(0);//End of row
              Serial_write(1);
          }
      }
      if(brl->buffer[buff_pos]==0)
        Serial_write(0);
      Serial_write(brl->buffer[buff_pos]);
      checksum=checksum+brl->buffer[buff_pos];
      x_pos++;buff_pos++;
  }
  printf("Done writting buffer.\n");
  return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
 if(cellsHaveChanged(previousCells, brl->buffer, buffer_size_max, NULL, NULL))
 {   
        Serial_write(2);
        writeWindow(brl,text);
 }
 else Serial_print("NO CHANGES");
}

/*
"""
def write_checksum(checksum):
    checksum_bytes=[]
    checksum_bytes.append(chr((checksum&0xFF000000)>>24))
    checksum_bytes.append(chr((checksum&0x00FF0000)>>16))
    checksum_bytes.append(chr((checksum&0x0000FF00)>>8))
    checksum_bytes.append(chr( checksum&0x000000FF))
    for checksum_byte in checksum_bytes:
        serialLogger.write(checksum_byte)

def read_checksum():
    checksum =  ord(serialLogger.read()) << 24
    checksum += ord(serialLogger.read()) << 16
    checksum += ord(serialLogger.read()) << 8
    checksum += ord(serialLogger.read())
    return checksum
    

def brltty_fill_buffer(buf,retry=False):
    if not retry:
        serialLogger.write(chr(2))
    #Tell the FCHAD device to enter read buffer mode.
    checksum = 0
    bytes_written=0
    bytes_written_since_last_check = 0
    for braille_byte in buff:#Write to the buffer.
        if braille_byte==0: serialLogger.write(chr(braille_byte))
        #keeping in mind that 0 is an escape charicter and must be written 
        #twice.  This is for a single row buffer support exists for more rows.
        #To go to the next row, use the escape sequence 00 01.  The C driver
        #should support multi row buffers, but since the current cursor driver
        #does not take advantage of this capability, I won't implement it now.
        serialLogger.write(chr(braille_byte))
        checksum = checksum + braille_byte
        bytes_written+=1
        bytes_written_since_last_check+=1
        if bytes_written_since_last_check>=10:
            print "Bytes written:"
            print bytes_written
            print "Checking progress."
            serialLogger.write(chr(0))
            serialLogger.write(chr(3))
            serialLogger.read()
            print "Confirmed."
            bytes_written_since_last_check=0
    serialLogger.write(chr(0))#Close read/write buffer mode.
    serialLogger.write(chr(2))
    write_checksum(checksum)
    print "Reading checksum."
    fchad_checksum=read_checksum()
    if not fchad_checksum == checksum:
        print "Checksum failed."
        print "FCHAD's checksum:"
        print fchad_checksum
        print "BRLTTY driver checksum:"
        print checksum
        
        #brltty_fill_buffer(buff,True)
"""
*/

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

/*
"""
def getKeyCode():
    while not serial.inWaiting()>2:1
    keycode = ord(serialLogger.read()) << 8
    return keycode + ord(serialLogger.read())


def readNextKey():
    keycode=getKeyCode()
    key = keycodes[keycode]()
    print keycode
    print key
    if keycode == 3:
        x=getKeyCode()
        y=getKeyCode()
        print str(x) + "," + str(y)

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
    serialLogger=loggingSerial(serialLogFile,serial)
    setting_manager=FCHAD_setting_manager(serialLogger)
    brltty_init()
    print "Filling buffer."
    buff=[]
    for i in range(setting_manager.settings["BUFFER_COLUMNS"][1]):
                                               #this is for a single row buffer
                                               #support exists for more rows.
        if i > 255: i=i-255*math.floor(i/255)
        buff.append(i)
    print "Buffer length:"
    print setting_manager.settings["BUFFER_COLUMNS"][1]
    brltty_fill_buffer(buff)
    print "Buffer filled."
    print "Reading key codes from serial."
    while True:readNextKey()
"""
*/
#if PYTHON
"""
#endif
