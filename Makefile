###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 1995-2011 by The BRLTTY Developers.
#
# BRLTTY comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/
#
# This software is maintained by Dave Mielke <dave@mielke.cc>.
###############################################################################

SRC_TOP = ../../../
SRC_DIR = .
# VPATH = .

BLD_TOP = ./../../../
BLD_DIR = .

O = o
X = 



default: all

include $(BLD_TOP)config.mk
include $(SRC_TOP)common.mk
include $(SRC_TOP)absdeps.mk
include $(SRC_DIR)/reldeps.mk

###############################################################################
# BRLTTY - A background process providing access to the console screen (when in
#          text mode) for a blind person using a refreshable braille display.
#
# Copyright (C) 2012 by Timothy Hobbs
# Copyright (C) 1995-2011 by The BRLTTY Developers.
#
# BRLTTY and the FCHAD software comes with ABSOLUTELY NO WARRANTY.
#
# This is free software, placed under the terms of the
# GNU General Public License, as published by the Free Software
# Foundation; either version 2 of the License, or (at your option) any
# later version. Please see the file LICENSE-GPL for details.
#
# Web Page: http://mielke.cc/brltty/ and http://brmlab.cz/user/timthelion
#
# This software is maintained by Timothy Hobbs <timothyhobbs@seznam.cz>.
###############################################################################

DRIVER_CODE = fc
DRIVER_NAME = FCHAD
DRIVER_COMMENT = For Fast CHAracter braille Displays
DRIVER_VERSION = 
DRIVER_DEVELOPERS = Timothy Hobbs 
include $(SRC_TOP)braille.mk

braille.$O:
	$(CC) $(BRL_CFLAGS) -c $(SRC_DIR)/braille.c

