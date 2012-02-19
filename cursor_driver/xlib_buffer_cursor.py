#! /usr/bin/python2
#
#Provide generalized class for interfacing with a cursor device for the 
#purposes of reading braille using a Fast CHAricter braille Display
#(FCHAD) type device.
#

from Xlib import X, display
import Xlib

import os
from time import sleep

import math

class buffer_cursor:
    def __init__(self, host, columns, rows):
        self._display            = display.Display()
        self._screen             = self._display.screen()
        self._root               = self._screen.root
        self._columns            = columns
        self._rows               = rows
        self._x_pos              = 0
        self._length             = 0
        self._host               = host
        self._screenWidth        = self._root.get_geometry().width
        self._buttons = [X.Button1,X.Button2, X.Button3,X.Button4, X.Button5]

        # setup xlib
        latency = 0.001
        for button in self._buttons:
            self._root.grab_button(button,
                    X.NONE,
                    self._root,
                    False,
                    X.GrabModeAsync,
                    X.GrabModeAsync,
                    X.NONE,
                    X.NONE)
        pid = os.fork()
        if pid:
            while 1:
                try:
                    event = self._root.display.next_event()
                    if event.detail == X.KeyPress:
                        self._root.send(event)
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
      x_pos=math.floor(self._mouseposx()/\
                (self._screenWidth/self._columns))
      x_pos=int(x_pos)
      if x_pos != self._x_pos:
        self._x_pos=x_pos
        self._host.update(x_pos)
