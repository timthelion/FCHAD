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
#define DEBUG             0

#define DEVICE_ID         "FCHAD"
#include "FCHAD_settings.h"

#include "FCHAD_codes.h"

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

void writeByteSide(unsigned char side, unsigned char character){
//Same as Serial_write_side in braille.c
    if(side==LEFT)
    {
        Serial.write((character&0b01111111)+0b10000000);
        Serial.write((character>>7)+0b10000000);
    }else{
        Serial.write(character&0b01111111);
        Serial.write(character>>7);
    }
}

void writeIntSide(unsigned char leftRight, int i){
    writeByteSide(leftRight, (uint8_t)((x & 0xFF00) >> 8) ) ;
    writeByteSide(leftRight, (uint8_t)x & 0x00FF ) ;    
}

int makeInt(unsigned char b1, unsigned char b2){
    b1 = (uint16_t)b1<<8;
    b2 =  (uint16_t)b2;
    return b1+b2;
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
void identify_mode_send_setting(byte setting,String value){
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
    displayChar(byte(255));
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
    
    //identify_mode_cleanup();
    //displayChar(byte(0));//Just to show that all the motors work and we finished
    //the init sequence with some degree of success.
}

////////////////////////////////////////////
///Idle mode////////////////////////////////
////////////////////////////////////////////
/*
The first bit of each byte recieved marks the channel(sender).
1 = LEFT  = from brltty
0 = RIGHT = from cursor driver
*/
static void read_buffer_mode_process_char    (unsigned char character);
static void set_cursor_pos_mode_process_char (unsigned char character);
static void recieve_key_mode_process_char    (unsigned char character);

unsigned char mode_brltty=IDLE_MODE;
unsigned char mode_cursor=IDLE_MODE;


bool          left_in_waiting       = 0;
bool          right_in_waiting      = 0;
unsigned char left_char_in_waiting  = 0;
unsigned char right_char_in_waiting = 0;

void process_char_left  (unsigned char character);
void process_char_right (unsigned char character);

void process_char(unsigned char character){//Written like this, so as to be able
//to copy paste this code directly between firmware and brltty driver.
    if(character & LEFT)
    {
        if(left_in_waiting)
        {
            character=left_char_in_waiting+((character&0b01111111)<<7);
            left_char_in_waiting=0;
            left_in_waiting=0;
            process_char_left(character);
        }else{
            left_char_in_waiting=(character&0b01111111);
            left_in_waiting=1;
        }
    }else{
        if(right_in_waiting){
            character=right_char_in_waiting+(character<<7);
            right_char_in_waiting=0;
            right_in_waiting=0;
            process_char_right(character);
        }else{
            right_char_in_waiting=character;
            right_in_waiting=1;
        }
    }
}

void process_char_left(unsigned char character){        
        switch(mode_brltty){
            case READ_WRITE_BUFFER_MODE:
                read_buffer_mode_process_char(character);
                break;
            case DISPLAY_CHAR_MODE:
                displayChar(character);
                break;
            case IDLE_MODE:
            default:
                switch(character){
                    case  SEND_RECIEVE_CURSOR_POS_MODE : 
                        send_cursor_pos();       break;
                    default:
                        mode_brltty = character; break;
                }
            break;
        }
}

void process_char_right(unsigned char character){
    switch(mode_cursor){
        case SEND_RECIEVE_CURSOR_POS_MODE:
            set_cursor_pos_mode_process_char(character);
            break;
        case SEND_RECIEVE_KEYCODE_MODE:
            recieve_key_mode_process_char(character);
            break;
        case DISPLAY_CHAR_MODE:
            displayChar(character);
            break;
        case IDLE_MODE:
        default:
            mode_cursor=character;
            break;
    }
}
///////////////////////////////////////////
///READ BUFFER MODE////////////////////////
///////////////////////////////////////////
int read_buffer_mode_x = 0;
int read_buffer_mode_y = 0;

/*
Bytes sent on the left channel while read buffer mode is activated will be
processed by this code.  Every byte recieved which is to be written to buffer
will be echo'd back to the brltty driver on the left channel.  The right channel
is reserved for keycodes which may be passed through from the cursor driver.
*/

static int buffer_index(int x, int y){return x+y*buffer_columns;}
void read_buffer_clear_eol(int xi,int yi){//Fill the rest of the row with 0s.
    xi++;
    while(in_buffer(xi,yi)){
        buffer[buffer_index(xi,yi)]=0;
        xi++;
    }
}

void read_buffer_clear_eob(int xi, int yi){
    while(in_buffer(xi,yi)){//Fill ther rest of the buffer with 0s.
        read_buffer_clear_eol(xi,yi);
        yi++;
        xi=0;
    }
}

void add_character_to_buffer(unsigned char character){
    writeByteSide(LEFT,character);//Write buffer mode confirmation characters
    //are send left sided to brltty.  Keycodes are sent right sided.
    buffer[buffer_index(read_buffer_mode_x,
                        read_buffer_mode_y)]=character;
    read_buffer_mode_x++;
}

#define READ_BUFFER_MODE_STAGE_READ_BYTES 0
#define READ_BUFFER_MODE_STAGE_ESCAPE     1
int read_buffer_mode_stage = READ_BUFFER_MODE_STAGE_READ_BYTES;
/*
There is so much I've learned to hate about "traditional" C.  I have seen many
projects that would have put these definitions in a separate <.h> file, and then
included that file at the top of the <.c>.  I find this only adds complications
in finding such definitions.  It reduces clairity and adds nothing.  If one is
to share such definitions.  The <.h> should atleast be included near the place
that they are used!
*/

static void read_buffer_mode_process_char    (unsigned char character)
{
    switch(read_buffer_mode_stage){
        case READ_BUFFER_MODE_STAGE_READ_BYTES:
            if(character == BUFFER_ESCAPE){
                read_buffer_mode_stage++;return;
            }
            add_character_to_buffer(character);
            break;
        case READ_BUFFER_MODE_STAGE_ESCAPE:
            switch(character){
                case BUFFER_ESCAPE:
                    add_character_to_buffer(BUFFER_ESCAPE);
                    read_buffer_mode_stage=READ_BUFFER_MODE_STAGE_READ_BYTES;
                    break;
                case BUFFER_EOL:
                    read_buffer_clear_eol(read_buffer_mode_x,read_buffer_mode_y);
                    read_buffer_mode_x=0;
                    read_buffer_mode_y++;
                    read_buffer_mode_stage=READ_BUFFER_MODE_STAGE_READ_BYTES;
                    if(in_buffer(read_buffer_mode_x,read_buffer_mode_y))break;
                    //EVIL ^_^
                case BUFFER_EOB:
                    read_buffer_clear_eob(read_buffer_mode_x,read_buffer_mode_y);
                    read_buffer_mode_x=0;
                    read_buffer_mode_y=0;
                    mode_brltty=IDLE_MODE;
                    read_buffer_mode_stage=READ_BUFFER_MODE_STAGE_READ_BYTES;
                    break;
                default:;
            }
            break;
    }
}

//////////////////////////////////////////////
////Mini Modes////////////////////////////////
//////////////////////////////////////////////
unsigned char cursor_pos_byte = 0;
unsigned char cursor_pos_bytes[4];
static void set_cursor_pos_mode_process_char (unsigned char character){
    if(cursor_pos_byte>=4){
        x=makeInt(cursor_pos_bytes[0],cursor_pos_bytes[1]);
        y=makeInt(cursor_pos_bytes[2],cursor_pos_bytes[3]);
        displayChar(buffer[buffer_index(x,y)]);
        cursor_pos_byte = 0;
        mode_cursor = IDLE_MODE;
    }else{
        cursor_pos_bytes[cursor_pos_byte]=character;
        cursor_pos_byte++;
    }
}

void send_cursor_pos(){
    writeIntSide(RIGHT,x);
    writeIntSide(RIGHT,y);
}

unsigned char receive_key_byte=0;
static void recieve_key_mode_process_char    (unsigned char character){
    writeByteSide(RIGHT,character);
    receive_key_byte++;
    if(receive_key_byte>=2){
        receive_key_byte=0;
        mode_cursor=IDLE_MODE;
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
  identify_mode();
}

void loop() {
    process_char(nextChar());
}
