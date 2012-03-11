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

#import "FCHAD_firmware/FCHAD_codes.h"

#include "FCHAD_firmware/FCHAD_settings.h"
int buffer_columns=20;//Defaults make testing easier.
int buffer_rows=1;    //Are never used in real life.
int buffer_size_max=800;
char * brltty_driver="BRLTTY_C";
char cursor_driver[lineBufferLength];
unsigned char character;

static unsigned char *previousCells = NULL; /* previous pattern displayed */
////////////////////////////////////////////////////////////////////////////////
//Arduino Serial////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define SERIAL 1
#define SERIAL_BAUD 9600
#define SERIAL_READY_DELAY 400
#define SERIAL_INPUT_TIMEOUT 100
#define SERIAL_WAIT_TIMEOUT 200
static GioEndpoint *gioEndpoint = NULL;
int serial_wait_time = 0;

void Serial_init(const char *identifier)
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


void Serial_print(char * line)
{
    #if SERIAL
    int size = strlen(line);
    gioWriteData(gioEndpoint, line, size);
    #else
    printf(line);
    #endif
}

void Serial_println(char * line)
{
    Serial_print(line);
    Serial_print("\n");
}

unsigned char Serial_read()
{
    #if SERIAL
    while(!gioAwaitInput(gioEndpoint,0));
    unsigned char byte;
    gioReadByte(gioEndpoint, &byte,0);
    //printf("<<\n");
    //printByte(byte);
    return byte;
    #else
    char * num[4];    
    gets(num);
    return atoi(num);
    #endif
    //Read one byte from serial.
}

void Serial_write(unsigned char byte)
{
    //#if SERIAL
    gioWriteData(gioEndpoint, &byte, 1);
    //#else
    //printf(">>\n");
    //printByte(byte);
    //#endif
    //Write one byte to serial.
}

void Serial_nextLine(char * buffer)
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
    if(byte== '\r')gioReadByte(gioEndpoint, &byte,0);//If we get line return, throw out the new line.
    buffer[i-1]='\0';
    if(i<=1)Serial_nextLine(buffer);
    #else
    gets(buffer);
    #endif
}

void waitFor(sender)
{
    char current_line[lineBufferLength];
    do {
        Serial_nextLine(current_line);
        printf(current_line);
    }while(0!=strcmp(current_line,sender));
}

unsigned char waitForByte(unsigned char byte)
{
    unsigned char introduction_byte = Serial_read();
    while(introduction_byte!=byte){
        printf("Got unexpected byte:\n");
        printByte(introduction_byte);
        introduction_byte =Serial_read();
    }
}

int readInt(){//Twobyte
  unsigned char cb1 = 10;
  cb1=Serial_read();
  unsigned char cb2 = Serial_read();
  int b1 = (uint16_t)cb1<<8;
  int b2 = (uint16_t)cb2;
  return b1+b2;
}

void writeInt(int checksum)//2 byte
{
    Serial_write((uint8_t)((checksum&0x0000FF00)>>8));
    Serial_write((uint8_t) (checksum&0x000000FF));
}


void writeLong(long checksum)
{
    Serial_write((uint8_t)((checksum&0xFF000000)>>24));
    Serial_write((uint8_t)((checksum&0x00FF0000)>>16));
    Serial_write((uint8_t)((checksum&0x0000FF00)>>8));
    Serial_write((uint8_t) (checksum&0x000000FF));
}

long readLong(){
  unsigned char cb1 = Serial_read();
  printf("Reading long, got first byte!\n");
  printByte(cb1);
  unsigned char cb2 = Serial_read();
  printf("Reading long, got second byte!");
  printByte(cb2);
  unsigned char cb3 = Serial_read();
  printf("Reading long, got third byte!");
  printByte(cb3);
  unsigned char cb4 = Serial_read();
  printf("Reading long, got fourth byte!");
  printByte(cb4);
  long b1 = (uint32_t)cb1<<24;
  long b2 = (uint32_t)cb2<<16;
  long b3 = (uint32_t)cb3<<8;
  long b4 = (uint32_t)cb4;
  return b1+b2+b3+b4;
}
////////////////////////////////////////////////////////////////////////////////
void printByte(unsigned char byte)
/*
Print byte to stdout a byte in a fashion which is tennable for debugging
purposes.  As a character, as a decimal number, and as binary layed out in
braille byte format.
*/
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

////////////////////////////////////////////////////////////////////////////////
///Send and recieve settings////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static void identify_mode_send_settings(){
  Serial_println("BRLTTY DRIVER");
  Serial_read();
  Serial_print(settings[BRLTTY_DRIVER]);
      Serial_print("=");
      Serial_println(brltty_driver);
  Serial_read();
  Serial_println(settings[END_HEADER]);
  Serial_read();
}

////////////////////////////////////////////////////////////////////////////////
unsigned char identify_mode_setting_eq(char * setting, char * buffer)
/*
Are the settings equal?  Why not use strcmp?  With this, we return an index and
not a compairison.
*/
{
  unsigned char i=0;
  while(1)
  {
    if(setting[i]=='\0')return i+1;
    if(setting[i]!=buffer[i])return 0;
    i++;
  }
}

////////////////////////////////////////////////////////////////////////////////
unsigned char identify_mode_which_setting(char * buffer)
/*
Return the index of the setting which has been passed to us.
*/
{
    unsigned char i;
    for(i=0;i<NumberOfSettings;i++){
        character=identify_mode_setting_eq(settings[i], buffer);
        if(character)
            return i;
    }
    return ERROR;
}

////////////////////////////////////////////////////////////////////////////////
static int identify_mode_receive_settings(char * sender, BrailleDisplay *brl)
/*
Recieve settings from the FCHAD device.
*/
{
    char current_line[lineBufferLength];
    waitFor(sender);
    unsigned char setting; unsigned char known_setting=1;
    while(1){
        Serial_nextLine(current_line); 
        setting = identify_mode_which_setting(current_line);
        switch(setting){
            case CURSOR_DRIVER:
            strcpy(&current_line[character],cursor_driver);break;
            case BRLTTY_DRIVER:
            strcpy(&current_line[character],brltty_driver);break;
            case BUFFER_COLUMNS:
            buffer_columns=atoi(&current_line[character]);break;
            case BUFFER_ROWS:
            buffer_rows=atoi(&current_line[character]);break;
            case BUFFER_SIZE_MAX:
            buffer_size_max=atoi(&current_line[character]);break;
            case SERIAL_WAIT_TIME:
            serial_wait_time=atoi(&current_line[character]);break;
            case END_HEADER:
            Serial_write(known_setting);
            brl->textColumns=buffer_columns;
            brl->textRows=buffer_rows;
            return 1;break;
            case ERROR:
            known_setting=0;
            printf("Unknown setting:");
            printf(current_line);
            printf("%d\n",character);break;
            default:;
        }
        Serial_write(known_setting);
    }
    return 0;//This should never happen.
}

static int identify_device(char * current_line)
{
  Serial_nextLine(current_line);
  if(strcmp(current_line,"FCHAD")!=0)
  {
     printf("Looking for an FCHAD device.  Got:\n");
     printf(current_line);
     printf(" instead.\n");
     printf("Try pressing the reset button on your device.\n");
     return 0;
  }else{
    printf("\nFCHAD device found.\n");
    return 1;
  }
}

static int init(BrailleDisplay *brl)
{
  char current_line[lineBufferLength];
  if(!identify_device(current_line))return 0;
  Serial_println("BRLTTY DRIVER - FCHAD?");Serial_read();
  if(!identify_mode_receive_settings("FCHAD", brl))return 0;
  printf("Done getting settings from FCHAD. Sending brltty driver settings.\n");
  identify_mode_send_settings();
  printf("Settings sent, waiting for cursor driver to start up.\n");
  waitFor("WAIT");
  printf("Cusor driver loading... will wait %d\n",serial_wait_time);
  approximateDelay(serial_wait_time*10);
  printf("Updating settings...\n");
  Serial_println("BRLTTY DRIVER - FCHAD?");
  if(!identify_mode_receive_settings("FCHAD",brl))return 0;
  previousCells = malloc(buffer_size_max);
  printf("Done with init.\n");
  return 1;//1 indicates success.
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

////////////////////////////////////////////////////////////////////////////////
//Write buffer//////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static char inbuffer(int x, int y, int columns, int rows)
{
    return x>=0 && y>=0 && x<columns && y<rows;
}

#define CHECKSUM_FAIL_RESET_FAIL    0
#define CHECKSUM_FAIL_RESET_SUCCESS 1
#define CHECKSUM_SUCCESS            2
int control_checksum(long checksum){
    writeChecksum(checksum);
    checksum_fchad=readChecksum();
    printf("Checksum read:%d\n",checksum_fchad);
    if(checksum_fchad==checksum){
        printf("Buffer written.\n");
        return 2;
    }else{
        printf("Checksum failed.\n");
        Serial_write(SEND_RECIEVE_KEYCODE_MODE);//Send a keycode to FCHAD device to see if it is
        //in idle mode.
        Serial_write(6);//The random
        Serial_write(7);//keycode.
        if(Serial_read()==6&&Serial_read()==7)
            return CHECKSUM_FAIL_RESET_SUCCESS;
        else{
            printf("Hmm, FCHAD device isn't in IDLE mode.");
            return CHECKSUM_FAIL_RESET_FAIL;
        }
    }
}
#define writeChecksum writeLong
#define readChecksum readLong
static int writeWindow(BrailleDisplay *brl, const wchar_t *text)
{
  long checksum=0;
  long checksum_fchad;
  int bytes_written_since_last_check=0;
  int fchad_bytes_read;
  unsigned char fchad_byte;
  int buff_pos=0;
  int x_pos=0;
  int y_pos=0;

  Serial_write(READ_WRITE_BUFFER_MODE);
  printf("Entering write buffer mode.\n");
  waitForByte(READ_WRITE_BUFFER_MODE);
  printf("Writting buffer.\n");
  while(1)
  {
      if(!inbuffer(x_pos,y_pos,brl->textColumns,brl->textRows))//EOL
      {
          x_pos=0;
          y_pos++;
          if(y_pos>=brl->textRows)//EOB
          {
              printf("EOB\n");
              Serial_write(BUFFER_ESCAPE);//End of buffer
              Serial_write(BUFFER_EOB);
              switch(control_checksum(checksum))
              {
                  case CHECKSUM_FAIL_RESET_FAIL:
                    return 0;
                    break;
                  case CHECKSUM_FAIL_RESET_SUCCESS:
                    writeWindow(brl, text);
                    break;
                  case CHECKSUM_SUCCESS:
                    return 1;
                    break;
              }
          }else
          {
              printf("EOL\n");
              Serial_write(BUFFER_ESCAPE);//End of row
              Serial_write(BUFFER_EOL);
          }
      }
      if(bytes_written_since_last_check>=10)
      {
          Serial_write(BUFFER_ESCAPE);
          Serial_write(BUFFER_INTERMEDIATE_CHECKSUM);
          writeInt(bytes_written_since_last_check);
          fchad_bytes_read = readInt();
          printf("  BRLTTY wrote %d bytes while FCHAD read %d bytes.\n",
           bytes_written_since_last_check, fchad_bytes_read);
          if(fchad_bytes_read != bytes_written_since_last_check)
          {
              printf("Bytes read check failed while writting to buffer.\n");
          }
          //printf("Checked %d\n",Serial_read());
          bytes_written_since_last_check=0;
      }
      bytes_written_since_last_check++;

      if(brl->buffer[buff_pos]==BUFFER_ESCAPE)
        Serial_write(BUFFER_ESCAPE);

      printf("Writting buffer possition: %d\n", buff_pos);
      printByte(brl->buffer[buff_pos]);
      Serial_write(brl->buffer[buff_pos]);
      fchad_byte = Serial_read();
      printf("Wrote %d recieved %d\n",brl->buffer[buff_pos],fchad_byte);

      if(fchad_byte!=brl->buffer[buff_pos]){
          printf("Woops, looks like we got interupted while writting to the buffer, restarting.\n");
          Serial_write(BUFFER_ESCAPE);
          Serial_write(BUFFER_EXIT_READ_WRITE_BUFFER_MODE);
          writeWindow(brl,text);
          return;
      }

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
        writeWindow(brl,text);
 }
 else Serial_print("NO CHANGES");
}

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

static int getKeyCode()
{
    return readInt();
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  return EOF;
  printf("Keycode:%d\n", readInt());
}
