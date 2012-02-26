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
from FCHADsettings import *
settings_to_send=["CURSOR_DRIVER","BUFFER_COLUMNS","BUFFER_ROWS"]#which of the settings in settings(by key),
#should we send to the FCHAD?


class braille_reader_host:
    def __init__(self,serial,):
        self._serial = serial
        settings["CURSOR_DRIVER"][1]="pythonXlib"
        
        self._columns            = 100
        settings["BUFFER_COLUMNS"][1]  = self._columns
        self._rows               = 1
        settings["BUFFER_ROWS"][1]  = self._rows

        self.serial_init()
        self._cursor = buffer_cursor(self,self._columns,self._rows)
    
    def serial_test_transfer_speed(self):
        print "Attacking the FCHAD with serial overflow\n"
        for t in range(10):
            delay=1.0/(10*t+1.0)
            print "Attacking every "+str(delay)+" seconds.\n"
            for c in ['a','t','t','a','c','k',str(t)]:
                time.sleep(delay)
                serialLogger.serialwrite(self._serial,chr(5))
                serialLogger.serialwrite(self._serial,chr(0))
                serialLogger.serialwrite(self._serial,c)
        print "Done with attack.\n"
    
    def serial_init(self):
        serialLogger.serialwrite(self._serial,"CURSOR DRIVER - FCHAD?\n")
        time.sleep(0.1)
        readSettings(self._serial,serialLogger)
        writeSettings(self._serial,serialLogger,"CURSOR DRIVER\n",settings_to_send)
            
    def update(self, pos):# fix this code to make it work with
                          # possitions > 255
         serialLogger.serialwrite(self._serial,chr(3))
         serialLogger.serialwrite(self._serial,chr(0))
         serialLogger.serialwrite(self._serial,chr(pos))
         serialLogger.serialwrite(self._serial,chr(0))
         serialLogger.serialwrite(self._serial,chr(0))
    
    def send_key(self,keycode):
        serialLogger.serialwrite(self._serial,chr(5))
        serialLogger.serialwrite(self._serial,chr(0))        
        serialLogger.serialwrite(self._serial,chr(keycode))#for keycodes up to 255
        
    def __delete__(self):
        self._serial.close()

help = "--help" in sys.argv or "-h" in sys.argv
log = "--log" in sys.argv
serialLogFile=None

if help:
    m = """Cursor driver for controlling FCHAD type devices.
    Start BRLTTY first!
    --log to log serial transactions to file.
    """
    print m
else:
    serial = Serial(sys.argv[1], 9600, timeout=1)
    if log:
        serialLogFile = open('serialLogFile.log', 'w')
    serialLogger=loggingSerial(serialLogFile)
    brh = braille_reader_host(serial)
