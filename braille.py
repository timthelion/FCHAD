from serial import Serial
import sys, os, time, math
from FCHADsettings import FCHAD_setting_manager
from loggingSerial import *

settings_to_send=["BRLTTY_DRIVER"]#which of the settings in settings(by key),
#should we send to the FCHAD?

keycodes = {
    1: lambda : "Previous line.",
    2: lambda : "Next line.",
    3: lambda : "Braille cell pressed at possition:"
    }

def waitFor(command):
    while not serial.inWaiting():1#wait for cursor driver to load.
    #If we get something else, we return what we got.
    received_command=serialLogger.readline()
    if received_command==command:
        return None
    else:
        return received_command

def brltty_init():
    DEVICE_ID = serialLogger.readline()
    print "Device ID:" + DEVICE_ID
    if not DEVICE_ID.startswith("FCHAD"):
        print "Not an FCHAD device."
        sys.exit()
    print "Gettings settings from FCHAD."
    setting_manager.readSettings("BRLTTY DRIVER - FCHAD?\n")
    print "Sending settings to FCHAD."
    setting_manager.settings["BRLTTY_DRIVER"][1]="DummyPython"
    setting_manager.writeSettings("BRLTTY DRIVER\n",settings_to_send)
    print "Successfully initialized(we hope), now go start up the cursor driver."
    
    wait_command = waitFor("WAIT")
    if not wait_command:
        time.sleep(setting_manager.settings["SERIAL_WAIT_TIME"][1]/100.0)
        print "Reading settings..."
        setting_manager.readSettings("BRLTTY DRIVER - FCHAD?\n")
    else:
        print "PROTOCOL ERROR, I EXPECTED 'WAIT' INSTEAD I GOT:"+wait_command
        sys.exit()
    print "Updated settings..."

def write_checksum(checksum):
    checksum_bytes=[]
    checksum_bytes.append(chr((checksum&0xFF000000)>>24))
    checksum_bytes.append(chr((checksum&0x00FF0000)>>16))
    checksum_bytes.append(chr((checksum&0x0000FF00)>>8))
    checksum_bytes.append(chr( checksum&0x000000FF))
    for checksum_byte in checksum_bytes:
        serialLogger.write(checksum_byte)

def read_checksum():
    checksum =  ord(serialLogger.read()) << 24
    checksum += ord(serialLogger.read()) << 16
    checksum += ord(serialLogger.read()) << 8
    checksum += ord(serialLogger.read())
    return checksum
    

def brltty_fill_buffer(buf,retry=False):
    if not retry:
        serialLogger.write(chr(2))
    #Tell the FCHAD device to enter read buffer mode.
    checksum = 0
    bytes_written=0
    bytes_written_since_last_check = 0
    for braille_byte in buff:#Write to the buffer.
        if braille_byte==0: serialLogger.write(chr(braille_byte))
        #keeping in mind that 0 is an escape character and must be written 
        #twice.  This is for a single row buffer support exists for more rows.
        #To go to the next row, use the escape sequence 00 01.  The C driver
        #should support multi row buffers, but since the current cursor driver
        #does not take advantage of this capability, I won't implement it now.
        serialLogger.write(chr(braille_byte))
        checksum = checksum + braille_byte
        bytes_written+=1
        bytes_written_since_last_check+=1
        if bytes_written_since_last_check>=10:
            print "Bytes written:"
            print bytes_written
            print "Checking progress."
            serialLogger.write(chr(0))
            serialLogger.write(chr(3))
            serialLogger.read()
            print "Confirmed."
            bytes_written_since_last_check=0
    serialLogger.write(chr(0))#Close read/write buffer mode.
    serialLogger.write(chr(2))
    write_checksum(checksum)
    print "Reading checksum."
    fchad_checksum=read_checksum()
    if not fchad_checksum == checksum:
        print "Checksum failed."
        print "FCHAD's checksum:"
        print fchad_checksum
        print "BRLTTY driver checksum:"
        print checksum
        
        #brltty_fill_buffer(buff,True)

def getKeyCode():
    while not serial.inWaiting()>2:1
    keycode = ord(serialLogger.read()) << 8
    return keycode + ord(serialLogger.read())


def readNextKey():
    keycode=getKeyCode()
    key = keycodes[keycode]()
    print keycode
    print key
    if keycode == 3:
        x=getKeyCode()
        y=getKeyCode()
        print str(x) + "," + str(y)

serial = Serial(sys.argv[1], 9600, timeout=1)
help = "--help" in sys.argv or "-h" in sys.argv
log = "--log" in sys.argv
serialLogFile=None

if help:
    m = """BRLTTY driver for controlling FCHAD type devices. Start this first
    then the cursor driver.  Then orca...  This driver is a dummy or prototype,
    if you actually want to use brltty, use the C version...
    
    First argument passed should be path of USB serial FCHAD device.
    Aka /dev/ttyUSB0.
    
    --log Log contents of serial transactions to file.
    
    -h,--help Display this message.
    """
    print m
else:
    if log:
        serialLogFile = open('serialLogFile.log', 'w')
    serialLogger=loggingSerial(serialLogFile,serial)
    setting_manager=FCHAD_setting_manager(serialLogger)
    brltty_init()
    print "Filling buffer."
    buff=[0b10000000,
          0b01000000,
          0b00100000,
          0b00010000,
          0b00001000,
          0b00000100]#test buffer for 6 bits.
    #for i in range(setting_manager.settings["BUFFER_COLUMNS"][1]):
    #                                           #this is for a single row buffer
    #                                           #support exists for more rows.
    #    if i > 255: i=i-255*math.floor(i/255)
    #    buff.append(i)
    print "Buffer length:"
    print setting_manager.settings["BUFFER_COLUMNS"][1]
    brltty_fill_buffer(buff)
    print "Buffer filled."
    print "Reading key codes from serial."
    while True:readNextKey()
