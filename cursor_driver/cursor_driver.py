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

class braille_reader_host:
    def __init__(self,serial,):
        self._serial = serial
        self._cursorDriverName="pythonXlib"
        
        self._columns            = 100
        self._rows               = 1

        self.serial_init()
        self._cursor = buffer_cursor(self,self._columns,self._rows)
    
    def serial_test_transfer_speed(self):
        print "Attacking the FCHAD with serial overflow\n"
        for t in range(10):
            delay=1.0/(10*t+1.0)
            print "Attacking every "+str(delay)+" seconds.\n"
            for c in ['a','t','t','a','c','k',str(t)]:
                time.sleep(delay)
                self._serial.write(chr(5))
                self._serial.write(chr(0))
                self._serial.write(c)
        print "Done with attack.\n"
    
    def serial_init(self):
        self._serial.write("CURSOR DRIVER - FCHAD?\n")
        self._serial.write("CURSOR DRIVER\n")
        self._serial.write("CURSOR_DRIVER="+self._cursorDriverName+"\n")
        self._serial.write("BUFFER_ROWS="+str(self._rows)+"\n")
        time.sleep(1)#TODO: Checksums, checksums, checksums!!!!
        self._serial.write("BUFFER_COLUMNS="+str(self._columns)+"\n")
        self._serial.write("END_HEADER\n")
        
    def update(self, pos):# fix this code to make it work with
                          # possitions > 255
         self._serial.write(chr(3))
         self._serial.write(chr(0))
         self._serial.write(chr(pos))
         self._serial.write(chr(0))
         self._serial.write(chr(0))
    
    def send_key(self,keycode):
        self._serial.write(chr(5))
        self._serial.write(chr(0))        
        self._serial.write(chr(keycode))#for keycodes up to 255
        
    def __delete__(self):
        self._serial.close()


serial = Serial(sys.argv[1], 9600, timeout=1)
help = "--help" in sys.argv or "-h" in sys.argv

if help:
    m = """Cursor driver for controlling FCHAD type devices.
    Start BRLTTY first!
    """
    print m
else:
    brh = braille_reader_host(serial)
