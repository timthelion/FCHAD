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
import time

settings={
    "BRLTTY_DRIVER" : [lambda raw_setting: raw_setting , ""],
    "CURSOR_DRIVER" : [lambda raw_setting: raw_setting ,""],
    "BUFFER_SIZE_MAX" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_COLUMNS" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_ROWS" : [lambda raw_setting: int(raw_setting), 0],
    "DOTCOUNT" : [lambda raw_setting: int(raw_setting), 0],
    "SERIAL_WAIT_TIME" : [lambda raw_setting: int(raw_setting), 0]}


class FCHAD_setting_manager:
    def __init__(self,serialLogger):
        self.settings=settings
        self.serialLogger=serialLogger

    def readSettings(self,question):
        if question:
            self.serialLogger.write(question)
        while 1: #Wait for the FCHAD to introduce itself.
            setting=self.serialLogger.readline()
            if setting == "FCHAD":
                break
        print "FCHAD introduced itself.  This is good."
        while 1:
            setting=self.serialLogger.readline()
            known_setting=1
            if setting == "END_HEADER": break
            psetting=setting.partition("=")
            try:
                value = self.settings[psetting[0]][0](psetting[2])
                if value:
                   self.settings[psetting[0]][1]=value
            except KeyError:
                print "Unknown setting:"+setting
                known_setting=0
            self.serialLogger.write(chr(known_setting))
                
            
    def writeSettings(self,driver, settings_to_send):
        self.serialLogger.write(driver)
        for setting in settings_to_send:
            self.serialLogger.write(setting+"="\
                +str(self.settings[setting][1])+"\n")
            self.serialLogger.read()
        self.serialLogger.write("END_HEADER\n")
