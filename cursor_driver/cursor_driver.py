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
    def __init__(self,serial):
        self._serial = serial
        self._cursorDriverName="pythonXlib"
        self._test = "--test" in sys.argv
        self._attack = "--attack" in sys.argv
        self._dry = "--no-cursor" in sys.argv
        
        self._columns            = 100
        self._rows               = 1

        if self._test or self._attack: self.serial_test_brltty_init()
        self.serial_init()
        if self._attack: self.serial_test_transfer_speed()

        if self._test: self.serial_test_brltty_fill_buffer()
        
        if not self._dry: 
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
            
            
    def serial_test_brltty_init(self):
        #We ignore the messages from the FCHAD device in this code.  It is meant
        #as a very quick informal test.
		self._serial.write("BRLTTY DRIVER - FCHAD?\n")
		self._serial.write("BRLTTY DRIVER\n")
		self._serial.write("END_HEADER\n")
        
    def serial_test_brltty_fill_buffer(self):
        self._serial.write(chr(2))
        self._serial.write(chr(0))
        self._serial.write(chr(0))
        checksum = 0
        for n in range(7):
            self._serial.write(chr(2**n))
            checksum = checksum + 2**n
        self._serial.write(chr(0))
        self._serial.write(chr(2))
        #write check sum
        checksum_byte1=chr((checksum&0b11111111000000000000000000000000)>>24)
        checksum_byte2=chr((checksum&0b00000000111111110000000000000000)>>16)
        checksum_byte3=chr((checksum&0b00000000000000001111111100000000)>>8)
        checksum_byte4=chr( checksum&0b00000000000000000000000011111111)
        print "Writting checksum to serial:" + str(checksum) 
        print "Byte 1:" + str(ord(checksum_byte1))
        print "Byte 2:" + str(ord(checksum_byte2))
        print "Byte 3:" + str(ord(checksum_byte3))
        print "Byte 4:" + str(ord(checksum_byte4))
        self._serial.write(checksum_byte1)
        self._serial.write(checksum_byte2)
        self._serial.write(checksum_byte3)
        self._serial.write(checksum_byte4)
        
    def serial_init(self):
        self._serial.write("CURSOR DRIVER - FCHAD?\n")
        self._serial.write("CURSOR DRIVER\n")
        self._serial.write("CURSOR_DRIVER="+self._cursorDriverName+"\n")
        self._serial.write("BUFFER_ROWS="+str(self._rows)+"\n")
        time.sleep(1)#TODO Ask someone about this.
                     #If this sleep isn't there, or if it's too short, the third
                     #setting line gets cut off by the nextLine() function of
                     #the FCHAD firmware.
                     #I presume there is some serial buffer in the arduino that
                     #is getting full, and I need to have the FCHAD driver write
                     #back after each line telling the cursor driver that it has
                     #processed the information.
        self._serial.write("BUFFER_COLUMNS="+str(self._columns)+"\n")
        self._serial.write("END_HEADER\n")
        

    def update(self, pos):# fix this code to make it work with
                          # possitions > 255
         self._serial.write(chr(3))
         self._serial.write(chr(0))
         self._serial.write(chr(pos))
         self._serial.write(chr(0))
         self._serial.write(chr(0))
         if self._test: print "UPDATING CURSOR POSSITION"+str(pos)

    def __delete__(self):
        self._serial.close()


serial = Serial(sys.argv[1], 9600, timeout=1)


pid = os.fork()
if pid: 
    serialLogFile = open('serialLogFile.log', 'w')
    serialLogFile.write("Beginning of log file:\n")
    while True: 
        while not serial.inWaiting():1
        char=serial.read()
        sys.stdout.write(char)
        serialLogFile.write(char)
        
else: brh = braille_reader_host(serial)
