#! /usr/bin/python2
#
#Provide generalized class for interfacing with a cursor device for the 
#purposes of reading braille using a Fast CHAricter braille Display
#(FCHAD) type device.
#
#TODO!!!!
#Rewrite this file in C!  Python xlib doesn't work.
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
        #Note that this does fail sometimes, because we interfere with
        #ourselves due to bad multithreading.  This can only be fixed by
        #rewritting in C to get around python-xlib misbehavior.
                    event = self._root.display.next_event()
                    if event.detail == X.KeyPress:
                        self._root.send(event)
                    else:
                        x_pos=self._getpos();
                        if x_pos != self._x_pos:
                            self._x_pos=x_pos
                            
                        if self._x_pos == -1:
                            self._host.send_key(1)
                        elif self._x_pos > self._columns:
                            self._host.send_key(2)
                        else:
                            self._host.send_key(3)
                            self._host.send_key(self._x_pos)
                            self._host.send_key(0)
                except AttributeError:
                    pass
                except ConnectionClosedError:
                    sys.exit()
        else:
            while 1:#Yes, I know this puts the CPU at 100%, I neet to rewrite in
                    #C because python's xlib has a certain bug, which grabs
                    #certain global hotkeys, when I add a hook to cursor 
                    #movement.  AKA grab_pointer grabs too much :O :/ :(
                    #And I can't even report this bug, because python_xlib is
                    #out of date.  Indeed, you may wonder why I wrote this in
                    #python2.  The reason, was, I couldn't find xlib bindings
                    #for python3 at all!
                sleep(latency)
                self._update()

    def _mouseposx(self):
        data = display.Display().screen().root.query_pointer()._data
        return data["root_x"]
    
    def _mouseposy(self):
        data = display.Display().screen().root.query_pointer()._data
        return data["root_y"]

    def _getpos(self):
      return int(math.floor(self._mouseposx()/\
            (self._screenWidth/(self._columns+2)))-1)
                #plus forward and back buttons...

    def _update(self):
      x_pos=self._getpos();
      if x_pos != self._x_pos:
         self._x_pos=x_pos
      if 0 <= x_pos <=self._columns:
         self._host.update(x_pos)
