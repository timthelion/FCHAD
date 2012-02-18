#! /usr/bin/python2
#
#Provide generalized class for interfacing with a cursor device for the 
#purposes of reading braille using a Fast CHAricter braille Display(FCHAD) type 
#device.
#

from Xlib import X, display
import Xlib

import os
from time import sleep

class braille_reader_cursor:
    def __init__(self, host):
        self._display            = display.Display()
        self._screen             = self._display.screen()
        self._root               = self._screen.root
        self._char_area          = (100,100)
        self._pos                = 0
        self._length             = 0
        self._host               = host
        # setup xlib
        latency = 0.001
        for button in buttons:
            root.grab_button(button,
                    X.NONE,
                    root,
                    False,
                    X.GrabModeAsync,
                    X.GrabModeAsync,
                    X.NONE,
                    X.NONE)
        pid = os.fork()
        if pid:
            while 1:
                try:
                    event = root.display.next_event()
                    if event.detail == X.KeyPress:
                        root.send(event)
                    else:
                        print event
                except AttributeError:
                    pass
                except ConnectionClosedError:
                    sys.exit()
        else:
            while 1:
                sleep(latency)
                self._update()

    def _mouseposx(self):
        data = display.Display().screen().root.query_pointer()._data
        return data["root_x"]
    
    def _mouseposy(self):
        data = display.Display().screen().root.query_pointer()._data
        return data["root_y"]

    def _update(self):
      pos=math.floor(self._mouseposx()/self._char_area[0])
      pos+=math.floor(self._mouseposy()/self._char_area[1])*\
                    math.floor(WidthOfScreen(self._screen)/self._char_area[1])
      pos = int(pos)
      if pos != self._pos:
        self._pos=pos
        self._host.update(pos)
     
    def set_length(self, length):
        self._length = length
