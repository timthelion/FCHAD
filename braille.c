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

#define BUFFER_COLUMNS 20
#define BUFFER_ROWS 1

/*
The MORSE_UNIT is determined in the pixels traveled by the mouse for each unit of morse time.

MORSE_SPACE_LENGTH
MORSE_DIT_LENGHT
MOSE_DAH_LENGTH
SPACE_BETWEEN_MORSE_CHARS
SPACE_BETWEEN_DIT_DAHS
MAX_EXPANDED_LENGTH

are all expressed in morse units.
*/
#define MORSE_UNIT 30 
#define MORSE_SPACE_LENGTH 4
#define MORSE_DIT_LENGTH 1
#define MORSE_DAH_LENGTH 2
#define SPACE_BETWEEN_MORSE_CHARS 1
#define SPACE_BETWEEN_DIT_DAHS 1 
/*This means that the space between two morse characters will be TWO units in length!  SPACE_BETWEEN_MORSE_CHARS+SPACE_BETWEEN_DIT_DAHS*/
#define MAX_EXPANDED_LENGTH MORSE_SPACE_LENGTH
/*
If MORSE_DAH_LENGTH was 3, for example, then MAX_EXPANDED_LENGTH would be 5: MORSE_DAH_LENGTH + SPACE_BETWEEN_DIT_DAHS + SPACE_BETWEN_MORSE_CHARS
*/
/*
MORSE_MAX_LENGHT and BRAILLE_MORSE_MAX_LENGTH define the number of bytes used to express one morse character and one braille morse character.
*/
#define MORSE_MAX_LENGTH 5
#define BRAILLE_MORSE_MAX_LENGTH 13

/*This buffer is filled in by brailleToBrailleMorse*/
wchar_t brailleMorseCode[BRAILLE_MORSE_MAX_LENGTH];

/*This buffer is filled in by wcharBrailleToMorse*/
wchar_t morse[BRAILLE_MORSE_MAX_LENGTH];


/*A chunk, is a string of textual morse code such as "..." which represents a single wchar_t. In the example case 's'. */
#define MORSE_CHAR_CHUNK_SIZE BRAILLE_MORSE_MAX_LENGTH+1
#define MORSE_BUFFER_LENGTH BUFFER_COLUMNS*MORSE_CHAR_CHUNK_SIZE
#define EXPANDED_MORSE_BUFFER_LENGTH MORSE_BUFFER_LENGTH*MAX_EXPANDED_LENGTH

static unsigned char expandedMorseBuffer[EXPANDED_MORSE_BUFFER_LENGTH];

/////////////////////
//Braille bits//////
///////////////////
/*
In brltty, the dots of braille characters are represented by the bits of an unsigned char.  You can look up masks for these bits in this array, with brailleBits[0] pointing to the first dot and following the weird braille dot numbering standard of:
14
25
36
78 ect.

or in our case n - 1 of that...

03
14
25
67
*/
static unsigned char brailleBits[8]=
 {
 1,//<<0
 1<<1,
 1<<2, 
 
       1<<3,
       1<<4,
       1<<5,
 1<<6, 
       1<<7};//<- art!

//////////////////////
//Morse table////////
////////////////////
/*
I didn't want to create a new struct for this.  So I decided to do one of those stupid "Cleaver" C things.  I found that all the morse code characters are 5 or less characters long.  So I made each string be 6 chars long.  First char is the key in our dictionary, the following 5 represent the morse code.  When a code is less than 5 dit/dahs long, I represent the unused space in the string by 0's...

Space is marked by ' '.
*/
#define NUMBER_OF_MORSE_LETTERS 37
static wchar_t wcharToMorseTable[37][7]=
 {L"a.-000",//0
  L"b-...0",//1
  L"c-.-.0",//2
  L"d-..00",//3
  L"e.0000",//4
  L"f..-.0",//5
  L"g--.00",//6
  L"h....0",//7
  L"i..000",//8
  L"j.---0",//9
  L"k-.-00",//10
  L"l.-..0",//11
  L"m--000",//12
  L"n-.000",//13
  L"o---00",//14
  L"p.--.0",//15
  L"q--.-0",//16
  L"r.-.00",//17
  L"s...00",//18
  L"t-0000",//19
  L"u..-00",//20
  L"v...-0",//21
  L"w.--00",//22
  L"x-..-0",//23
  L"y-..-0",//24
  L"z--..0",//25
  L"1.----",//26
  L"2..---",//27
  L"3...--",//28
  L"4....-",//29
  L"5.....",//30
  L"6-....",//31
  L"7--...",//32
  L"8---..",//33
  L"9----.",//34
  L"0-----",//35
  L"  0000" //36
  };

/*
Since many characters cannot be represented at all with the morse code we need some way of presenting them to our users.  So I made a way of escaping special characters with an unused 5 dit/dah code.  After that follows an 8dit/dah braille character encoded into dit/dah form and in the order of the weird braille dot numbering system described above...  This is not a good way of presenting data, but I don't have time right now to do it correctly.  Perhaps a better way to encorporate this would be to actually, for example, write out asterisc for a '*'.  This would also be slow, but at least it might be learnable...

This function returns a 13 byte string.(period, it is ALWAYS 13 bytes, without any null escape sequence at the end :D)

*/
void brailleToBrailleMorse(unsigned char brailleChar)
{
 /*
 I really don't like this system!  It's WAY to slow(in terms of reading spead, not computational preformance)!!!  Just to test...
 */
 wchar_t * brailleEscapeChar = L"..-..";
 /*
 This is just some unused morse sequence.  It could be anything.  I also considered using a double long dah for the escape code.
 */
 memcpy(brailleMorseCode, brailleEscapeChar,5*sizeof(wchar_t));
 /* braille charicters represented like this after the escape char:
 e 10     -.
   01  -> .-  -> -...-... [(-..),(.-.),(..)]
   00     ..
   00     ..
   */
 #define INDEX_OFFSET_FROM_BRAILLE MORSE_MAX_LENGTH
 int indexInBrailleMorseCodeArray = 5;
 while(indexInBrailleMorseCodeArray<BRAILLE_MORSE_MAX_LENGTH){
  if(brailleChar & brailleBits[indexInBrailleMorseCodeArray-INDEX_OFFSET_FROM_BRAILLE]){
  /*
  Because I couldn't think if a more complicated way of counting to 8 and &'ing the bitmask of each bit in the braille byte.
  */
   brailleMorseCode[indexInBrailleMorseCodeArray]=L'-';
   /*Dahs for raised dots*/
  }else{
   brailleMorseCode[indexInBrailleMorseCodeArray]=L'.';
   /*Dits for unraised dots*/
  }
  indexInBrailleMorseCodeArray++;
 }
}

/*
Returns a 5 byte array representing the morse code of the character given, OR, the string "00000" in the case that the given wchar_t does not have a standard US morse code equivalent.
*/
wchar_t * lookUpMorse(wchar_t wchar){
 int indexInWCharToMorseTable = 0;
 while(indexInWCharToMorseTable<NUMBER_OF_MORSE_LETTERS){
  if(wcharToMorseTable[indexInWCharToMorseTable][0]==wchar){
   return(&wcharToMorseTable[indexInWCharToMorseTable][1]);
  }
  indexInWCharToMorseTable++;
 }
 return(L"00000");
}

/*
This function converts a braille array + a wchar array to "Braille-morse code.  This is almost the same as Morse code, except special characters that don't appear in morse code get represented by Braille characters.

The return result is a string(array of chars) which is 13 bytes long in which dits are marked by '.' and dahs are marked by '-'.  Any extraneous bytes are marked out by 0's.
*/
wchar_t * wcharBrailleToMorse(unsigned char braille, const wchar_t text)
{
 wchar_t * morseFromWChar = lookUpMorse(text);

 /*If there was no morse character which represents that wchar_t character, we change over to printing escaped morse-braille*/
 if(morseFromWChar[0]=='0'){
  brailleToBrailleMorse(braille);
  return(brailleMorseCode);
 }else{/*If we were lucky, and our wchar_t character IS represented by a morse one, we still have to fill out the rest of the buffer with '0's*/
  memcpy(morse,morseFromWChar,MORSE_MAX_LENGTH*sizeof(wchar_t));
  int indexInMorse=MORSE_MAX_LENGTH;
  while(indexInMorse<BRAILLE_MORSE_MAX_LENGTH){
   morse[indexInMorse]='0';
   indexInMorse++;
  }
  return(morse);
 }
 return(L"Error");//WE NEVER GET THIS FAR...
}

/*
We use this repeatedly later to add multiple instances of the same character into a buffer...
buffer = buffer we are writting multiple instances of the same byte into.
index = start index in buffer.
multiplyer = how many times we shoule write our byte to the buffer.
filler = what byte should we write to the buffer
*/
void expandTo(unsigned char * buffer, int * index,int multiplyer,unsigned char filler){
 int counter = 0;
 while(counter<multiplyer){
  buffer[*index]=filler;
  (*index)++;
  counter++;
 }
}
/*
Every time the brltty buffer is updated, we will reload a new morse buffer...
This is a two step process.  First we convert the braille/wchar text to morse code.  Then we expand the morse code into an array of booleans which denote on off states of our electromagnet.

[Of course this algorithm would be more efficient if our wcharToMorseTable was expanded automatically at driver load time.  I didn't think about this though, and I don't think that the preformance difference is worth my coding time to go back and change it.]

When I said booleans back there, I meant unsigned chars where 0 = 0 and 1 = 255.  This is because of the way we consume these bytes.  We send them to our FCHAD device as if they were normal braille characters.

*/
void loadMorseBuffer(unsigned char * braille,const wchar_t * text)
{
 //the +1 is there to leave room to add a single null byte between chunks.
 wchar_t morseBuffer[MORSE_BUFFER_LENGTH];
 int indexInBrlttyBuffer = 0;
 int indexInMorseBuffer = 0;
 wchar_t * currentChunk;
 int indexInCurrentChunk;//Used later in the loop, established now to save on memory allocation time.
 while(indexInBrlttyBuffer<BUFFER_COLUMNS){
  /*
  We convert the braille-wchar_t character to morse code.
  */
  currentChunk=wcharBrailleToMorse(braille[indexInBrlttyBuffer],text[indexInBrlttyBuffer]);
  //printf("currentChunk:%Ls",currentChunk);
  /*
  Now we have to figure out how long our current chunk actually is.
  */
  indexInCurrentChunk=0;
  while(indexInCurrentChunk<MORSE_CHAR_CHUNK_SIZE){
   if(currentChunk[indexInCurrentChunk]=='0')break;
   indexInCurrentChunk++;
  }
  /*Now we copy our current chunk into our morseBuffer*/
  memcpy(&morseBuffer[indexInMorseBuffer],currentChunk,(indexInCurrentChunk+1)*sizeof(wchar_t));
  indexInMorseBuffer=indexInMorseBuffer+indexInCurrentChunk;
  /*Add a space between the chars*/
  morseBuffer[indexInMorseBuffer]=L'\0';
  indexInBrlttyBuffer++;
  //printf("Working %d/%d\n",indexInBrlttyBuffer,BUFFER_COLUMNS);
 }
 /*Now we fill out the rest of the buffer with blanks*/
 while(indexInMorseBuffer<MORSE_BUFFER_LENGTH){
  morseBuffer[indexInMorseBuffer++]=L' ';
 }
 printf("START\n");
 //printf("Converted to morse, expanding now.\n");
  /*Now we expand this buffer out from morse code (dits and dahs expressed by the characters '.','-','\0'[for spaces between characters] and ' '[spaces between words/normal spaces]) into FCHAD code to be sent directly to the device expressed by unsigned bytes 0 and 255.*/
 int indexInExpandedMorseBuffer=0;
 indexInMorseBuffer=0;
 while(indexInMorseBuffer<MORSE_BUFFER_LENGTH)
 {
  //printf("Working %d/%d\n",indexInMorseBuffer,MORSE_BUFFER_LENGTH);
  switch(morseBuffer[indexInMorseBuffer]){
   case L'\0':
    //printf("Matched \\0\n");
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,SPACE_BETWEEN_MORSE_CHARS,0);
    break;
   case L' ':
    //printf("Matched ' '\n");
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,MORSE_SPACE_LENGTH,0);
    break;
   case L'.':
    //printf("Matched '.'\n");
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,MORSE_DIT_LENGTH,255);
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,SPACE_BETWEEN_DIT_DAHS,0);
    break;
   case L'-':
    //printf("Matched '-'\n");
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,MORSE_DAH_LENGTH,255);
    expandTo(expandedMorseBuffer,&indexInExpandedMorseBuffer,SPACE_BETWEEN_DIT_DAHS,0);
    break;
   default:
    printf("OH MY!!!  WHAT SHALL I DO???  I don't know how to handle a morse code represented by the character:%c\n",morseBuffer[indexInMorseBuffer]);
    printf("%d",morseBuffer[indexInMorseBuffer]);
    break;
  }
  indexInMorseBuffer++;
 }
 /*Fill out the rest of the buffer with blanks.*/
 while(indexInExpandedMorseBuffer<EXPANDED_MORSE_BUFFER_LENGTH){
  expandedMorseBuffer[indexInExpandedMorseBuffer]=0;
 }
 printf("END\n");
}
////////////////////////////////////////////////////////////////////////////////
//Arduino Serial////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define SERIAL 0
#define SERIAL_BAUD 9600
#define SERIAL_READY_DELAY 400
#define SERIAL_INPUT_TIMEOUT 100
#define SERIAL_WAIT_TIMEOUT 200
static GioEndpoint *gioEndpoint = NULL;
int serial_wait_time = 1;//TODO???

static void printByte(unsigned char byte)
{
    printf("CHAR:%c\n",byte);
    printf("DEC:%d\n",byte);
    if(byte & brailleBits[0]) printf("1" ); else printf ("0" );
    if(byte & brailleBits[3]) printf("1\n"); else printf ("0\n");
  
    if(byte & brailleBits[1]) printf("1" ); else printf ("0" );
    if(byte & brailleBits[4]) printf("1\n"); else printf ("0\n");
  
    if(byte & brailleBits[2]) printf("1" ); else printf ("0" );
    if(byte & brailleBits[5]) printf("1\n"); else printf ("0\n");
  
    if(byte & brailleBits[6]) printf("1" ); else printf ("0" );
    if(byte & brailleBits[7]) printf("1\n"); else printf ("0\n");
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
 //printf("Loading event loop\n");
 Window rootwin;
 int scr;
 GC gc;//TODO, what is this value and why was it here?
 if(!(dpy=XOpenDisplay(NULL))) {
  fprintf(stderr, "ERROR: could not open display\n");
  exit(1);
 }
 
 scr = DefaultScreen(dpy);
 rootwin = RootWindow(dpy, scr);
    
 XGrabPointer(dpy, rootwin, True,
                 ButtonPressMask |
                 PointerMotionMask,
               GrabModeAsync,
               GrabModeAsync,None,
               None,
               CurrentTime);
 XEvent e;
 int x,y,new_x,new_y=0;
 while(1) {
  XNextEvent(dpy, &e);
  if(e.type==MotionNotify){
  /*If the mouse moved, we update our x cordinate(our place in the buffer), then we send new information about what character is being displayed to our FCHAD device.*/
   new_y=0;
   new_x=e.xkey.x/(1024/(EXPANDED_MORSE_BUFFER_LENGTH+1));//TODO get screen size!!!
   if(x!=new_x||y!=new_y){
    printf("x:%d y:%d,i:%d\n",e.xkey.x,e.xkey.y,new_x);
    x=new_x;y=new_y;
    if(x>0 && x<=EXPANDED_MORSE_BUFFER_LENGTH){
     Serial_write(expandedMorseBuffer[x-1]);
    }
   }
  }else if(e.type==ButtonPress){
  /*If the mouse button was pressed, we have a choice of 4 possible actions.
  1)We can ungrab the mouse if the keypress was in the top left corner of the screen.   This allows other applications to gain mouse grab.  Hopfully, this hack can be replaced by proper use of XI2.
  2)We can send a braille key press signal for the currently highlighed cell in our buffer.
  3 and 4) we can send scroll signals to brltty if we are at the edges of the screen.
  */
   if(e.xkey.x<100&&e.xkey.y<100){
    printf("Ungrab\n");
    XUngrabPointer(dpy, CurrentTime);
    XGrabButton(dpy, AnyButton,None, rootwin, False, ButtonPressMask, GrabModeAsync, GrabModeAsync, None, None);
    XNextEvent(dpy, &e);
    printf("Grab\n");
    XGrabPointer(dpy, rootwin, True,
                ButtonPressMask |
                PointerMotionMask,
            GrabModeAsync,
            GrabModeAsync,None,
            None,
            CurrentTime);
   }else if(x>0&&x<EXPANDED_MORSE_BUFFER_LENGTH){
    key=(unsigned char)x;//TODO TODO, we shouldn't base this off x.  We need to create a NEW array which links cordinates in the expandedMorseBuffer to the cordinates of the brl->braillebuffer...
   }else if(x==0){
    key=SCROLL_LEFT;
   }else if(x==(EXPANDED_MORSE_BUFFER_LENGTH+1)){
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
  brl->textColumns=BUFFER_COLUMNS;
  brl->textRows=BUFFER_ROWS;

  //XLib thread
  printf("Creating xlib thread.\n");
  pthread_create(&event_loop,NULL, &run_event_loop,NULL);
  printf("Everything loaded.\n");
  return 1;
}

static void
brl_destruct (BrailleDisplay *brl) {
 XCloseDisplay(dpy);
}

static int
brl_writeWindow (BrailleDisplay *brl, const wchar_t *text) {
 //printf("Writing window\n");
 if(!text){
  printf("No text.\n");
 }else{
  //printf("Text found.\n");
  //wprintf(L"%ls\n\n",text);
  //wprintf(L"FOOOOOOO\n");
  //printf("There is nothing inbetween here and \"writting window\"\n");
  loadMorseBuffer(brl->buffer,text);
  //printf("Window written\n");
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
   return 0;
}

static int
brl_readCommand (BrailleDisplay *brl, KeyTableCommandContext context) {
 if(in_waiting)
 {
  in_waiting=0;
  switch(key){
   case SCROLL_LEFT:
    printf("SCROLL_LEFT\n");
    enqueueCommand(BRL_CMD_FWINLT);break;
   case SCROLL_RIGHT:
    printf("SCROLL_RIGHT\n");
    enqueueCommand(BRL_CMD_FWINRT);break;
   default:
    printf("Keypress%d \n",key);
    enqueueCommand(BRL_BLK_ROUTE + key);
  }
 }
 return EOF;
}
