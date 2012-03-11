 #
 # FCHAD software.  Software for X11 to act as a cursor for any arduino 
 # based FCHAD device.  
 #
 # Copyright (C) 2012 by Timothy Hobbs
 #
 # This hardware project would not have been possible without the guidance
 # and support of the many good people at brmlab <brmlab.cz> as well as
 # material support in terms of office space and machinery used for soldering
 # and manufacture of electric components.
 #
 # The FCHAD software comes with ABSOLUTELY NO WARRANTY.
 #
 # This is free software, placed under the terms of the
 # GNU General Public License, as published by the Free Software
 # Foundation; either version 2 of the License, or (at your option) any
 # later version. Please see the file LICENSE-GPL for details.
 #
 # Web Page: http://brmlab.cz/user/timthelion
 #
 # This software is maintained by Timothy Hobbs <timothyhobbs@seznam.cz>
 #

import sys, os, time

from serial import Serial
from xlib_buffer_cursor import buffer_cursor

sys.path.insert(0, '../')#GRRRRR :( :(
from loggingSerial import *
from FCHADsettings import FCHAD_setting_manager
settings_to_send=["CURSOR_DRIVER","BUFFER_COLUMNS","BUFFER_ROWS"]
#which of the settings in settings(by key),
#should we send to the FCHAD?

class braille_reader_host:
    def __init__(self,serial):
        self._serial = serial

        self._setting_manager=FCHAD_setting_manager(serialLogger)
        self._setting_manager.settings["CURSOR_DRIVER"][1]="pythonXlib"
        self._columns            = 20
        self._setting_manager.settings["BUFFER_COLUMNS"][1]  = self._columns
        self._rows               = 1
        self._setting_manager.settings["BUFFER_ROWS"][1]  = self._rows
        print "Initializing..."
        self.serial_init()
        print "Initialization successfull."
        time.sleep(self._setting_manager.settings["SERIAL_WAIT_TIME"][1]/100.0)
        if nap:    
            self.nap()
        if not dry:
            self._cursor = buffer_cursor(self,self._columns,self._rows)
        else:
            while True:
                self.displayChar(ord(raw_input()))
        
    def serial_init(self):
        serialLogger.write("CURSOR DRIVER - FCHAD?\n")
        time.sleep(0.1)
        self._setting_manager.readSettings(None)
        self._setting_manager.writeSettings("CURSOR DRIVER\n",settings_to_send)
            
    def update(self, pos_x,pos_y):
         print "Updating at cordinates " + str(pos_x)+"X"+str(pos_y)
         serialLogger.writeRight(chr(4))
         serialLogger.writeRight(chr(pos_x >> 8))
         serialLogger.writeRight(chr(pos_x &  0X00FF))
         serialLogger.writeRight(chr(pos_y >> 8))
         serialLogger.writeRight(chr(pos_y >> 0X00FF))
    
    def send_key(self,keycode):
        print "Sending key:"
        print keycode
        serialLogger.writeRight(chr(5))
        serialLogger.writeRight(chr(keycode >> 8))
        serialLogger.writeRight(chr(keycode &  0X00FF))
        
    def nap(self):
        print "Napping..."
        for i in range(10000):
            displayChar(255)
            time.sleep(0.05)
            displayChar(0)
            time.sleep(0.05)
        print "Good morning!"
        
    def displayChar(self, char):
        serialLogger.writeRight(chr(6))
        serialLogger.writeRight(chr(char))
        
    def __delete__(self):
        self._serial.close()

help = "--help" in sys.argv or "-h" in sys.argv
log = "--log" in sys.argv
nap = "--nap" in sys.argv
dry = "--dry" in sys.argv
serialLogFile=None

if help:
    m = """Cursor driver for controlling FCHAD type devices.
    Start BRLTTY first!
    --log to log serial transactions to file.
    --nap use the LED's to do an apha state nap.
    --dry initialize, but do not send any positions or keycodes.
    """
    print m
else:
    serial = Serial(sys.argv[1], 9600, timeout=1)
    if log:
        serialLogFile = open('serialLogFile.log', 'w')
    serialLogger=loggingSerial(serialLogFile,serial)
    brh = braille_reader_host(serial)
