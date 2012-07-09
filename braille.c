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

#include<X11/Xlib.h>
#include <pthread.h>

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

int buffer_columns=20;
int buffer_rows=1;

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

static void printByte(unsigned char byte)
{
    printf("CHAR:%c\n",byte);
    printf("DEC:%d\n",byte);
    if(byte & (1)) printf("1" ); else printf ("0" );
    if(byte & (1<<3)) printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<1)) printf("1" ); else printf ("0" );
    if(byte & (1<<4)) printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<2)) printf("1" ); else printf ("0" );
    if(byte & (1<<5)) printf("1\n"); else printf ("0\n");
  
    if(byte & (1<<6)) printf("1" ); else printf ("0" );
    if(byte & (1<<7)) printf("1\n"); else printf ("0\n");
}



int Serial_init(const char *identifier){
//COPY PASTE from connectResource() in the Voyager driver...
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



unsigned char Serial_read(){
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

void Serial_write(unsigned char byte){
    #if SERIAL
    gioWriteData(gioEndpoint, &byte, 1);
    #else
    printf(">>\n");
    printByte(byte);
    #endif
    //Write one byte to serial.
}
////////////////////////////////////////////////////////////////////////////////
//KEYS//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define SCROLL_LEFT  255
#define SCROLL_RIGHT 254
////////////////////////////////////////////////////////////////////////////////
//XLIB//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
Display *dpy;
pthread_t * event_loop;
char in_waiting;
unsigned char key;

void run_event_loop(){
 	Window rootwin;
 	int scr;
 	GC gc;
 	if(!(dpy=XOpenDisplay(NULL))) {
 		fprintf(stderr, "ERROR: could not open display\n");
 		exit(1);
 	}
 
 	scr = DefaultScreen(dpy);
 	rootwin = RootWindow(dpy, scr);
    
    XGrabPointer(dpy, rootwin, True,
                 ButtonPressMask |
                 PointerMotionMask |
                 FocusChangeMask |
                 EnterWindowMask |
                 LeaveWindowMask,
               GrabModeAsync,
               GrabModeAsync,None,
               None,
               CurrentTime);
 	XEvent e;
 	int x,y,new_x,new_y;
    while(1) {
    XNextEvent(dpy, &e);
	if(e.type==MotionNotify){
	    new_y=0;
	    new_x=e.xkey.x/(1024/(buffer_columns+1));//TODO get screen size!!!
	    if(x!=new_x||y!=new_y)
	    {
	        printf("x:%d y:%d,i:%d\n",e.xkey.x,e.xkey.y,new_x);
	        if(x>0&&x<buffer_columns){
	            Serial_write(previousCells[x-1]);
	        }
	        x=new_x;y=new_y;
	    }
	}else if(e.type==ButtonPress){
	    if(e.xkey.x<100&&e.xkey.y<100){
	        printf("Ungrab\n");
	        XUngrabPointer(dpy, CurrentTime);
	        XGrabButton(dpy, AnyButton,None, rootwin, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
	        XNextEvent(dpy, &e);
	        printf("Grab\n");
            XGrabPointer(dpy, rootwin, True,
                ButtonPressMask |
                PointerMotionMask |
                FocusChangeMask |
                EnterWindowMask |
                LeaveWindowMask,
            GrabModeAsync,
            GrabModeAsync,None,
            None,
            CurrentTime);
	    }else if(x>0&&x<buffer_columns)
	    {
	        key=(unsigned char)x;
	    }else if(x==0){
	        key=SCROLL_LEFT;
	    }else if(x==(buffer_columns+1)){
	        key=SCROLL_RIGHT;
	    }
        in_waiting=1;
	}else{
	    printf("Other event\n");
	}
 }
}

////////////////////////////////////////////////////////////////////////////////
//BRLTTY FUNCTIONS//////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static int
brl_construct (BrailleDisplay *brl, char **parameters, const char *device) {
  //SERIAL
  #if SERIAL
  Serial_init(device);
  #endif
  //BRLTTY
  brl->textColumns=buffer_columns;
  brl->textRows=buffer_rows;
  previousCells = malloc(buffer_columns*buffer_rows);

  //XLib thread
  pthread_create(&event_loop,NULL, &run_event_loop,NULL);
  return 1;
}

static void
brl_destruct (BrailleDisplay *brl) {
    XCloseDisplay(dpy);
}

static int writeWindow(BrailleDisplay *brl, const wchar_t *text){
    return 1;
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
 if(cellsHaveChanged(previousCells, brl->buffer, buffer_columns, NULL, NULL,NULL))
 {   
        writeWindow(brl,text);
 }
 return 1;
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

static int getKeyCode(){
   // return readInt();
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
  if(in_waiting)
  {
    in_waiting=0;
    switch(key){
      case SCROLL_LEFT: printf("SCROLL_LEFT\n");
                        enqueueCommand(BRL_CMD_FWINLT);break;
      case SCROLL_RIGHT: printf("SCROLL_RIGHT\n");
                         enqueueCommand(BRL_CMD_FWINRT);break;
      default:printf("Keypress%d \n",key);
                enqueueCommand(BRL_BLK_ROUTE + key);
    }
  }
  return EOF;
}
