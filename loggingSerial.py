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

import serial

###################################################
###Evil nasty logging serial functions#############
###################################################
class loggingSerial:
    def __init__(self,serialLogFile,serialdev):
        self.serialdev=serialdev
        if serialLogFile:serialLogFile.write("Beginning of log file:\n")
        self.serialLogFile = serialLogFile
        
    def read(self):
        while not self.serialdev.inWaiting():1
        byte_string = self.serialdev.read()
        if byte_string:
            x=byte_string[0]
        else:
            print "Hmm, seems we made a mistake, and no bytes where availible, trying again... "
            return self.read()
        if self.serialLogFile:
            self.serialLogFile.write("##READ##")
            self.serialLogFile.write(x)
            self.serialLogFile.write("\n")
        return x
    
    def readline(self):
        try:
            x=self.serialdev.readline()
        except serial.serialutil.SerialException:
            print "SerialException on serialreadline..."
            return self.serialdev.readline()
        x=x.partition("\r")[0]#get rid of line returns...
        if self.serialLogFile:
            self.serialLogFile.write("##READ_LINE##")
            self.serialLogFile.write(x)
            self.serialLogFile.write("\n")
        return x
    
    def write(self, x):
        self.serialdev.write(x)
        print "writting char "
        if self.serialLogFile:
            self.serialLogFile.write("##WRITE##")
            self.serialLogFile.write(x)
            self.serialLogFile.write("\n")

    def writeRight(self, x):
        self.write(chr(ord(x)&0b01111111))
        self.write(chr(ord(x)>>7))
