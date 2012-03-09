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
#define DEVICE_ID         "FCHAD"
#define DEBUG             0

#define ERROR             0
#define CURSOR_DRIVER     1
#define BRLTTY_DRIVER     2
#define BUFFER_SIZE_MAX   3
#define BUFFER_COLUMNS    4
#define BUFFER_ROWS       5
#define DOTCOUNT          6
#define SERIAL_WAIT_TIME  7
#define END_HEADER        8

#define NumberOfSettings  9

String settings[]={
  "ERROR",
  "CURSOR_DRIVER",
  "BRLTTY_DRIVER",
  "BUFFER_SIZE_MAX",
  "BUFFER_COLUMNS",
  "BUFFER_ROWS",
  "DOTCOUNT",
  "SERIAL_WAIT_TIME",
  "END_HEADER"};

#define dotCount 6
const int dotPins[] = {13,11,9,7,5,3};

#define serialWaitTime 300

#define buffer_size_max 700
byte buffer[buffer_size_max];  //The buffer of braile bytes to be displayed.
int  buffer_columns=0;
int  buffer_rows=0;
int x=0;
int y=0;

String cursor_driver="";
String brltty_driver="";

#define lineBufferLength 40 
char current_line[lineBufferLength];
byte character;

//////////////////////////////////////////
///Standard functions/////////////////////
//////////////////////////////////////////

boolean in_buffer(int xi,int yi){
    return xi<buffer_columns&&yi<buffer_rows;
}

//////////////////////////////////////////
///Debug//////////////////////////////////
//////////////////////////////////////////
void debug_message(String message,byte level)
//For level use 1 unless your message is *really* annoying in which case use 2
//and set DEBUG to 2. 
{
    if(DEBUG >= level)
    {
        Serial.print(message);
    }
}

void debug_message_ln(String message,byte level)
{
    if(DEBUG >= level)
    {
        Serial.println(message);
    }
}

///////////////////////////////////////////
///Serial//////////////////////////////////
///////////////////////////////////////////
byte nextChar()
{
  while(!Serial.available());//Block untill next char is here.
  return Serial.read();
}

byte nextLine()
{
    while(!Serial.available());//Block untill next char is here.
    byte e = Serial.readBytesUntil('\n', current_line, lineBufferLength);
    current_line[e]='\0';
    if(current_line[e-1]=='\r')current_line[e-1]='\0';
    return e;
}
void waitFor(char * line)
{
    int index;
    do{
        nextLine();
    }while(strcmp(current_line ,line));
}

void writeInt(int x){
    Serial.write( (uint8_t)((x & 0xFF00) >> 8) ) ;
    Serial.write( (uint8_t)x & 0x00FF ) ;
}

int readInt(){
    int b1 = (uint16_t)nextChar()<<8;
    int b2 =  (uint16_t)nextChar();
    return b1+b2;
}

long readLong(){
  long b1 = (uint32_t)nextChar()<<24;
  long b2 = (uint32_t)nextChar()<<16;
  long b3 = (uint32_t)nextChar()<<8;
  long b4 = (uint32_t)nextChar();
  return b1+b2+b3+b4;
}

static void writeLong(long checksum)//Write a long to serial
{
    Serial.write((uint8_t)((checksum&0xFF000000)>>24));
    Serial.write((uint8_t)((checksum&0x00FF0000)>>16));
    Serial.write((uint8_t)((checksum&0x0000FF00)>>8));
    Serial.write((uint8_t) (checksum&0x000000FF));
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
  debug_message("Displaying character:",1);
    debug_message_ln(String(character),1);
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

///////////////////////////////////////////
///Modes///////////////////////////////////
///////////////////////////////////////////


///////////////////////////////////////////
///Identify mode///////////////////////////
///////////////////////////////////////////

void identify_mode_send_setting(byte setting,String value)
{
    byte tries = 2;
    do{
        Serial.print(settings[setting]);
        Serial.print("=");
        Serial.println(value);
        tries--;
    }while(!nextChar()&&tries);
}

void identify_mode_send_settings(){
  Serial.println(DEVICE_ID);
  
  identify_mode_send_setting(CURSOR_DRIVER,     cursor_driver);
  identify_mode_send_setting(BRLTTY_DRIVER,     brltty_driver);
  identify_mode_send_setting(BUFFER_SIZE_MAX,   String(buffer_size_max,DEC));
  identify_mode_send_setting(BUFFER_COLUMNS,    String(buffer_columns,DEC));
  identify_mode_send_setting(BUFFER_ROWS,       String(buffer_rows,DEC));
  identify_mode_send_setting(DOTCOUNT,          String(dotCount,DEC));
  identify_mode_send_setting(SERIAL_WAIT_TIME,  String(serialWaitTime,DEC));
  
  Serial.println(settings[END_HEADER]);
  nextChar();
}

byte identify_mode_setting_eq(String setting){
  byte setting_length=setting.length();
  for(byte i=0;i<setting_length;i++)
  {
    if(setting[i]!=current_line[i])return 0;
  }
  return setting_length+1;
}

byte identify_mode_which_setting(){
    for(byte i=0;i<NumberOfSettings;i++){
        character=identify_mode_setting_eq(settings[i]);
        if(character)
            return i;
    }
    return ERROR;
}

void identify_mode_receive_settings(char * driver){
    waitFor(driver);
    byte setting;
    while(true){
        nextLine();
        setting = identify_mode_which_setting();
        debug_message("Setting:",1);debug_message(String(setting),1);
                                    debug_message_ln(current_line,1);
        debug_message("Index:",1);debug_message_ln(String(character),1);
        switch(setting){
            case CURSOR_DRIVER:
                cursor_driver=String(&current_line[character]);break;
            case BRLTTY_DRIVER:  
                brltty_driver=String(&current_line[character]);break;
            case BUFFER_COLUMNS:
                buffer_columns=atoi(&current_line[character]);break;
            case BUFFER_ROWS:
                buffer_rows=atoi(&current_line[character]);break;
            case END_HEADER:
                Serial.write(byte(1));
                return;break;
            case ERROR:   
                    debug_message("Unknown setting:",1);
                        debug_message_ln(current_line,1);
                    if(DEBUG)break;
                    Serial.write(byte(0));
                    break;
            default:;
        }
        if(!setting==ERROR)Serial.write(byte(1));
    }
}

void identify_mode_cleanup(){
    free(settings);
    free(&brltty_driver);
    free(&cursor_driver);
    free(current_line);
}

void identify_mode(){
    Serial.println(DEVICE_ID);
    waitFor("BRLTTY DRIVER - FCHAD?");
    identify_mode_send_settings();
    identify_mode_receive_settings("BRLTTY DRIVER");
    
    waitFor("CURSOR DRIVER - FCHAD?");
    Serial.println("WAIT");
    //if(Serial.available())Serial.println("Unexpected input.");
    identify_mode_send_settings();
    identify_mode_receive_settings("CURSOR DRIVER");
    
    delay(serialWaitTime);
    
    waitFor("BRLTTY DRIVER - FCHAD?");    
    identify_mode_send_settings();
    
    identify_mode_cleanup();
    displayChar(byte(255));//Just to show that all the motors work and we finished
    //the init sequence with some degree of success.
}

////////////////////////////////////////////
///Idle mode////////////////////////////////
////////////////////////////////////////////
void idle_mode_next(){
   character = nextChar();
   switch(character){
      case  2:read_buffer_mode();break;
      case  3:set_cursor_pos();break;
      case  4:send_cursor_pos();break;
      case  5:receive_key();break;
      case  6:displayChar(nextChar());break;
  }
}

///////////////////////////////////////////
///READ BUFFER MODE////////////////////////
///////////////////////////////////////////
void read_buffer_clear_eol(int xi,int yi)
{//Fill the rest of the row with 0s.
    xi++;
    while(in_buffer(xi,yi))
    {
        buffer[xi+yi*buffer_columns]=0;
        xi++;
    }
}

void read_buffer_clear_eob(int xi, int yi)
{//Fill ther rest of the buffer with 0s.
    while(in_buffer(xi,yi))
    {
        read_buffer_clear_eol(xi,yi);
        yi++;
        xi=0;
    }
}

#define readChecksum  readLong
#define writeChecksum writeLong
void process_eob(int xi, int yi, long checksum_fchad) 
{
    long checksum_brltty;
    read_buffer_clear_eob(xi,yi);
    debug_message_ln("End of buffer.",1);
    writeChecksum(checksum_fchad);//This should be flipped.  We should read then
    checksum_brltty=readChecksum();//write, but that hangs.  I honestly don't
    //why.  It doesn't matter though, really.
    if(checksum_fchad!=checksum_brltty){
        debug_message_ln("CHECK SUM FAILED",1);
        debug_message(String(checksum_fchad),1);debug_message_ln("!=",1);
        debug_message_ln(String(checksum_brltty),1);
    }
}

void read_buffer_mode(){
  Serial.write(2);
  long checksum_fchad=0;
  debug_message_ln("Entering read buffer mode.",1);
  int xi=0;
  int yi=0;
  int bytes_read_since_last_check=0;
  while(true)
  {
    character = nextChar();
    if(character==0)
    {
      character=nextChar();
      switch(character){
        case 0: break; //0
        case 1: //EOL
            read_buffer_clear_eol(xi,yi);
            xi=0;yi++;
            if(!in_buffer(xi,yi)){
debug_message_ln("ERROR END OF BUFFER REACHED WHILE GOING TO NEXT LINE",1);
            return;}
            break;
        case 2: process_eob(xi,yi,checksum_fchad); return;
        case 3: writeInt(bytes_read_since_last_check);
                bytes_read_since_last_check=0;
                readInt();
                //if(bytes_read_since_last_check!=readInt())return;
                character = nextChar();
                break;
        case 4: return;//Just go back to idle mode.
        default:;
      }
    }
    //if(!in_buffer(xi,yi)){
    // debug_message_ln("ERROR END OF BUFFER REACHED WHILE WRITTING TO BUFFER",1);
    // return;
    //}
    buffer[xi+yi*buffer_columns]=character;
    Serial.write(character);
    checksum_fchad+=character;
    bytes_read_since_last_check ++;
    debug_message("Adding character ",1);debug_message_ln(String(character),1);
    xi++;
  }
}

//////////////////////////////////////////////
////Mini Modes////////////////////////////////
//////////////////////////////////////////////
void set_cursor_pos(){
    x=readInt();
    y=readInt();
    debug_message("CURSOR POSSITION SET TO:",1);
        debug_message(String(x),1);
        debug_message(",",1);
        debug_message_ln(String(y),1);
    if(in_buffer(x,y))
        displayChar(buffer[x+y*buffer_columns]);
    else{x=0;y=0;
        debug_message("CURSOR ROUTED TO INVALID REGION WITHIN BUFFER.",1);}
}

void send_cursor_pos(){
    writeInt(x);
    writeInt(y);
}

void receive_key(){
    Serial.write(nextChar());
    Serial.write(nextChar());
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
  identify_mode();
}

void loop() {
    idle_mode_next();
}
