/*
 * FCHAD firmware.  Firmware for the arduino to control any FCHAD device.  
 *
 * Copyright (C) 2012 by Timothy Hobbs
 *
 * This hardware project would not have been possible without the guidance
 * and support of the many good people at brmlab <brmlab.cz> as well as
 * material support in terms of office space and machinery used for soldering
 * and manufacture of electric components.
 *
 * The FCHAD software comes with ABSOLUTELY NO WARRANTY.
 *
 * This is free software, placed under the terms of the
 * GNU General Public License, as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any
 * later version. Please see the file LICENSE-GPL for details.
 *
 * Web Page: http://brmlab.cz/user/timthelion
 *
 * This software is maintained by Timothy Hobbs <timothyhobbs@seznam.cz>
 */
#define dotCount 6
const int dotPins[] = {13,12,11,10,9,8};

////////////////////////////////////////
///FRAMES///////////////////////////////
////////////////////////////////////////
void checkForFrameAndReact();
void sendFrame(uint16_t length, unsigned char type, unsigned char subType, unsigned char * information);
void writeEscapedByte(unsigned char byte);
unsigned char readEscapedByte();

#define START_FLAG 02
#define END_FLAG 03
#define ESCAPE_CHAR 033
/*
[START_FLAG(1 octet) |
 LEN(2 octets)       |
 TYPE(1 octet)       |
 SUBTYPE(1 octet)    |
 INFORMATION         |
 XOR(1 octet)        |
 END_FLAG(1 octet)   ]
*/
void checkForFrameAndReact()
{
/*
 We check if there are bytes waiting for us in our serial buffer.  If there are, we read that byte.  If we are not confused, this byte should be a START_FLAG.  If we ARE confused, we ignore the byte and go on eating.
 */
 if(Serial.available()&&Serial.read()==START_FLAG)
 {
  unsigned char xorChecksum=0;
  uint16_t length;
  unsigned char type;
  unsigned char subType;
  unsigned char byteInMouth; 
  unsigned char indexInInformationBuffer=0;
  unsigned char endOfFrame=0;

  /*Now we read the frames header.*/
  byteInMouth=readEscapedByte();
  xorChecksum^=byteInMouth;
  length=(uint16_t)byteInMouth<<8;
  byteInMouth=readEscapedByte();
  xorChecksum^=byteInMouth;
  length+=byteInMouth;
  unsigned char information[length+1];
  
  type=readEscapedByte();
  xorChecksum^=type;
  subType=readEscapedByte();
  xorChecksum^=subType;
  /*Now we read our information buffer*/
  while(indexInInformationBuffer<length){
   byteInMouth=readEscapedByte();
   information[indexInInformationBuffer++]=byteInMouth;
   xorChecksum^=byteInMouth;
  }
  /*If our indexInInformationBuffer is not the same as our length+ourChecksum, we DROP the frame.*/
  if(indexInInformationBuffer!=length){
   return;
  }
  /*If our checksum failed, we DROP the frame.*/
  if(xorChecksum!=0){
   return;
  }
  /*If our frame was not closed off by an END_FLAG we DROP the frame*/
  if(nextChar()!=END_FLAG){
   return;
  }
  /*If everything was successfull, then we call the handling code to deal with this new bit of information.  We pass this handling code the length, the type, and the subType. AND WE DO NOT FORGET, THAT THE INFORMATION BUFFER IS CONSTRAINED BY THE LENGTH AND NOT ANY KIND OF NULL TERMINATOR*/
  handleFrame(length,type,subType,information);
 }
}

void sendFrame(uint16_t length, unsigned char type, unsigned char subType, unsigned char * information){
 Serial.write(START_FLAG);

 unsigned char xorChecksum=0;
 unsigned char byteInArse=(length&0xFF00ul)>>8;
 xorChecksum^=byteInArse;
 writeEscapedByte(byteInArse);

 byteInArse=length&0x00FFul;
 xorChecksum^=byteInArse;
 writeEscapedByte(byteInArse);

 xorChecksum^=type;
 writeEscapedByte(type);

 xorChecksum^=subType;
 writeEscapedByte(subType);

 unsigned char indexInInformationBuffer=0;
 while(indexInInformationBuffer<length){
  xorChecksum^=information[indexInInformationBuffer];
  writeEscapedByte(information[indexInInformationBuffer]);
 }
 writeEscapedByte(xorChecksum);
 Serial.write(END_FLAG);
}

void writeEscapedByte(unsigned char byte){
 switch(byte){
  case START_FLAG:
  case END_FLAG:
  case ESCAPE_CHAR:
   Serial.write(ESCAPE_CHAR);
  default:
   break;
 }
 Serial.write(byte);
}

unsigned char readEscapedByte(){
 unsigned char byte=nextChar();
 if(byte==ESCAPE_CHAR){
  return(nextChar());
 }return(byte);
}

////////////////////////////////////////
void handleFrame(uint16_t length,unsigned char type, unsigned char subType, unsigned char * information){}
////////////////////////////////////////
byte nextChar()
{
  while(!Serial.available());//Block untill next char is here.
  return Serial.read();
}
//////////////////////////////////////////
///Display functions//////////////////////
//////////////////////////////////////////
void dot_display_init()
{
  for(int n = 0;n<dotCount;n++)
  {
    pinMode(dotPins[n], OUTPUT);
  }
}

void displayChar(byte character)
{
  for(int n = 0;n<dotCount;n++)
  {
   if((1 << n) & character)
   {
     digitalWrite(dotPins[n],HIGH);
   }else
   {
     digitalWrite(dotPins[n],LOW);
   }
  }
}

//////////////////////////////////////////////
////Standard initializers/////////////////////
//////////////////////////////////////////////
void setup()
{
  // initialize the serial communication:
  Serial.begin(9600);
  // initialize the the pins as outputs:
  dot_display_init();
}

void loop() {
    displayChar(nextChar());
}
