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

#define ERROR             0
#define CURSOR_DRIVER     1
#define BRLTTY_DRIVER     2
#define BUFFER_SIZE_MAX   3
#define BUFFER_COLUMNS    4
#define BUFFER_ROWS       5
#define DOTCOUNT          6
#define END_HEADER        7

#define NumberOfSettings  8

String settings[]={
  "ERROR",
  "CURSOR_DRIVER",
  "BRLTTY_DRIVER",
  "BUFFER_SIZE_MAX",
  "BUFFER_COLUMNS",
  "BUFFER_ROWS",
  "DOTCOUNT",
  "END_HEADER"};

#define dotCount 6
const int dotPins[] = {13,11,9,7,5,3};

#define serialWaitTime 300 //ms

#define buffer_size_max 800
byte buffer[buffer_size_max];  //The buffer of braile bytes to be displayed.
int  buffer_columns=0;
int  buffer_rows=0;
int x=0;
int y=0;

String cursor_driver="";
String brltty_driver="";

#define lineBufferLength 40 
char current_line[lineBufferLength];
byte charicter;

//////////////////////////////////////////
///Standard functions/////////////////////
//////////////////////////////////////////
byte stringeq(char* string1, char* string2, char eol, byte length)
{
    byte i;
    for(i=0; i < length; i++)
    {
        if(string1[i]==eol&&string2[i]==eol)return i;
        //Serial.print("string1[i]");Serial.println(string1[i]);
        //Serial.print("string2[i]");Serial.println(string2[i]);
        if(string1[i]!=string2[i])return 0;
    }
    return i;
}

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
    return e;
}

void waitFor(char * line)
{
    byte index;
    do{
        nextLine();
        index = stringeq(current_line ,line,'\0',lineBufferLength);
    }while(!index);
}

void writeInt(int x){
    Serial.write( (uint8_t)((x & 0xFF00) >> 8) ) ;
    Serial.write( (uint8_t)x & 0x00FF ) ;
}

//////////////////////////////////////////
///Display functions//////////////////////
//////////////////////////////////////////

void displayChar(byte charicter)
{
  for(int n = 0;n<dotCount;n++)
  {
   if((1 << n) & charicter)
   {
     digitalWrite(dotPins[n],HIGH);
   }else
   {
     digitalWrite(dotPins[n+8],LOW);
   }
  }  
}

boolean in_buffer(){
    return x<buffer_columns&&y<buffer_rows;
}


///////////////////////////////////////////
///Modes///////////////////////////////////
///////////////////////////////////////////


///////////////////////////////////////////
///Identify mode///////////////////////////
///////////////////////////////////////////
/////TODO//////////////////////////////////
/////Add chech sums to identify mode///////
///////////////////////////////////////////
void identify_mode_send_settings(){
  Serial.println("FCHAD");
  Serial.print(settings[CURSOR_DRIVER]);
      Serial.print("=");
      Serial.println(cursor_driver);
 
  Serial.print(settings[BRLTTY_DRIVER]);
      Serial.print("=");
      Serial.println(brltty_driver);
      
  Serial.print(settings[BUFFER_SIZE_MAX]);
      Serial.print("=");
      Serial.println(String(buffer_size_max,DEC));

  Serial.print(settings[BUFFER_COLUMNS]);
      Serial.print("=");
      Serial.println(String(buffer_columns,DEC));
      
  Serial.print(settings[BUFFER_ROWS]);
      Serial.print("=");
      Serial.println(String(buffer_rows,DEC));
 
  Serial.print(settings[DOTCOUNT]);
      Serial.print("=");
      Serial.println(String(dotCount,DEC));

  Serial.println(settings[END_HEADER]);
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
        charicter=identify_mode_setting_eq(settings[i]);
        if(charicter)
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
        //Serial.print("Setting:");Serial.print(setting);
        //Serial.println(current_line);
        //Serial.print("Index:");Serial.println(charicter);
        switch(setting){
            case CURSOR_DRIVER:  cursor_driver=String(&current_line[charicter]);break;
            case BRLTTY_DRIVER:  brltty_driver=String(&current_line[charicter]);break;
            case BUFFER_COLUMNS: buffer_columns=atoi(&current_line[charicter]);break;
            case BUFFER_ROWS:    buffer_rows=atoi(&current_line[charicter]);break ;
            case END_HEADER:     return;break;
            case ERROR:         Serial.print("Unknown setting:");
                                Serial.println(current_line);break;
            default:;
        }
    }
}

void identify_mode_cleanup(){
    free(settings);
    free(&brltty_driver);
    free(&cursor_driver);
    free(current_line);
}

void identify_mode(){
    waitFor("BRLTTY DRIVER - FCHAD?");
    identify_mode_send_settings();
    identify_mode_receive_settings("BRLTTY DRIVER");
    waitFor("CURSOR DRIVER - FCHAD?");
    Serial.print("WAIT ");Serial.println(serialWaitTime);
    identify_mode_send_settings();
    identify_mode_receive_settings("CURSOR DRIVER");
    delay(serialWaitTime);
    identify_mode_send_settings();
    identify_mode_cleanup();
  }

////////////////////////////////////////////
///Idle mode////////////////////////////////
////////////////////////////////////////////
void idle_mode_next(){
   charicter = nextChar();
   switch(charicter){
      case  2:read_buffer_mode();break;
      case  3:set_cursor_pos();break;
      case  4:send_cursor_pos();break;
      case  5:receive_key();break;
      case  6:displayChar(0);break;//Sleep
  }
}

///////////////////////////////////////////
///READ BUFFER MODE////////////////////////
///////////////////////////////////////////
void read_buffer_clear_eol()
{
    x++;
    while(in_buffer())
    {
        buffer[x+y*buffer_columns]=0;
        x++;
    }
}

void read_buffer_clear_eob()
{
    while(in_buffer())
    {
        read_buffer_clear_eol();
        y++;
        x=0;
    }
}

void read_buffer_die(String message){
    Serial.println(message);
}

long read_buffer_read_check_sum(){
  return Serial.read()*16777216+Serial.read()*65536+
         Serial.read()*256+Serial.read();
}

void read_buffer_mode(){
  long check_sum=0;
  while(true)
  {
    charicter = nextChar();
    check_sum+=charicter;
    if(charicter==0)
    {
      charicter=nextChar();
      switch(charicter){
        //case 0: //0
        case 1: //EOL
            read_buffer_clear_eol();
            x=0;
            y++;
            if(!in_buffer()){
                read_buffer_die("ERROR END OF BUFFER REACHED");
                return;
            }
        case 2: //EOB
            read_buffer_clear_eob();
            if(check_sum!=read_buffer_read_check_sum()){
            //when check sum fails try again.
              Serial.write(byte(0));
              read_buffer_mode();
            }
            return;
      }
    }
    x++;
    if(!in_buffer())
        return read_buffer_die("ERROR END OF BUFFER REACHED");
    buffer[x+y*buffer_columns]=charicter;
  }
}

//////////////////////////////////////////////
////Mini Modes////////////////////////////////
//////////////////////////////////////////////
void set_cursor_pos(){
    x=Serial.read()*256+Serial.read();
    y=Serial.read()*256+Serial.read();
    if(in_buffer())
        displayChar(buffer[x+y*buffer_columns]);
    else{x=0;y=0;
        Serial.println("CURSOR ROUTED TO INVALID REGION WITHIN BUFFER.");
    }
}

void send_cursor_pos(){
    writeInt(x);
    writeInt(y);
}

void receive_key(){
    Serial.write(Serial.read());
    Serial.write(Serial.read());
}

//////////////////////////////////////////////
////Standard initializers/////////////////////
//////////////////////////////////////////////
void setup()
{
  // initialize the serial communication:
  Serial.begin(9600);
  // initialize the the pins as outputs:
  for(int n = 0;n<dotCount;n++)
  {
    pinMode(dotPins[n], OUTPUT);
  }
  identify_mode();
}

void loop() {
    idle_mode_next();
}
