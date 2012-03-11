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
const int dotPins[] = {11,13,9,7,3,5};


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
