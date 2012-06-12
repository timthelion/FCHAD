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

#define numberOfAnalogIns 3
const int analogInPins[numberOfAnalogIns] = {A0,A1,A2};
// Analog input pin that the potentiometer is attached to.

#define THRESHHOLD -70
//The maximum difference in analog value across the test period required to detect contact between finger and sensor.
#define testINTERVAL 5
//The how long between tests in milisecconds?

int sensorValues[numberOfAnalogIns] = {0,0,0};
// Value read from the pot.
int sensorValuesPre[numberOfAnalogIns] = {0,0,0};
// Value read from the pot.
int difs[numberOfAnalogIns] = {0,0,0};
// The differences between the analog values, between
// The beginning of the test period and the end.

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

unsigned long timeThen = 0;
unsigned long timeNow;

unsigned char currentlyTouchedSensor = 0;
bool touchUnchanged;
//We don't use multitouch support, even though we have it.
//We only send a signal back to BRLTTY if the focused sensor has
//changed and the previously touched sensor is no longer being
//touched.

void setup()
{
  // initialize the serial communication:
  Serial.begin(9600);
  // initialize the the pins as outputs:
  dot_display_init();
}

void loop() {
  //If a new character is availiable, display it.
  if(Serial.available()){
  	displayChar(Serial.read());
  }
  //If so much time has passed, check if the
  //focused cell has changed.  If so, send a message
  //to brltty.
  timeNow=millis();
  //If the time has overflown, or is greater than testINTERVAL milliseconds.
  if(timeNow < timeThen || timeNow-timeThen > testINTERVAL){
   // read the analog in value:
   for(unsigned char i = 0;i < numberOfAnalogIns; i++){
    sensorValues[i] = analogRead(analogInPins[i]);
    difs[i]=sensorValues[i]-sensorValuesPre[i];
    sensorValuesPre[i]=sensorValues[i];
    if(difs[i]<THRESHHOLD){
     if(i!=currentlyTouchedSensor){
      touchUnchaged = 1;
     }else{
      currentlyTouchedSensor=i;
     }
    }
    difs[i]=0;
   }
   //If the currently touched sensor HAS changed, 
   if(!touchUnchanged){
   	Serial.write(currentlyTouchedSensor);
   }
  }
}
