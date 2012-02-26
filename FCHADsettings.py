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
import serial,time

settings={
    "BRLTTY_DRIVER" : [lambda raw_setting: raw_setting , ""],
    "CURSOR_DRIVER" : [lambda raw_setting: raw_setting ,""],
    "BUFFER_SIZE_MAX" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_COLUMNS" : [lambda raw_setting: int(raw_setting), 0],
    "BUFFER_ROWS" : [lambda raw_setting: int(raw_setting), 0],
    "DOTCOUNT" : [lambda raw_setting: int(raw_setting), 0]
}


def readSettings(serialdev,serialLogger):
    psettings={}
    while 1:
        setting=serialLogger.serialreadline(serialdev)
        setting=setting.partition("\r")[0]#stupid pyserial gets not only the line, 
                                      #but the line return as well...
        if setting == "FCHAD":
            break
    while 1:
        setting=serialLogger.serialreadline(serialdev)
        setting=setting.partition("\r")[0]#stupid pyserial gets not only the line, 
                                          #but the line return as well...
        if setting == "END_HEADER": break
        psetting=setting.partition("=")
        if settings[psetting[0]][0](psetting[2]):
            try:
                settings[psetting[0]][1]=settings[psetting[0]][0](psetting[2])
            except KeyError:
                print "Unknown setting:"+setting
        
def writeSettings(serialdev,serialLogger,driver, settings_to_send):
    serialLogger.serialwrite(serialdev,driver)
    for setting in settings_to_send:
        serialLogger.serialwrite(serialdev,setting+"="+str(settings[setting][1])+"\n")
        time.sleep(0.1)
    serialLogger.serialwrite(serialdev,"END_HEADER\n")
