WHAT is an FCHAD?
An FCHAD is a Fast CHaracter braille Display.  That is, it's a braille
display that displays one character at a time, and can change which
character it is displaying sufficiently quickly to still be useful.  That
is, about 12-35 times a second.

The user then controls a cursor-like device to move through the text and
read. That is, left hand on the FCHAD display, right hand on the mouse.

This is an experimental project in human computer interaction.  The results
of it, if any, will be free open source hardware/software for the
blind/vision impaired.

What parts make up the whole?

FCHAD: The arduino device that controls 6-9 LED lights, motors, solenoids,
etc., which are able to update which braille character they are displaying
at faster than 12 characters per seccond.

CURSOR: The user-controlled cursor, which selects which character will be
displayed by the FCHAD.

BRLTTY: The program which sends text from the computer to the FCHAD for
display.

Up there, I gave as an example LED lights.  Why LED lights, if the users are
blind?  Most "blind" people are not blind.  They can at least see the
direction of light.  I hope to experiment, to see if 6 lights shone close to
the eyes of such individuals can lead to faster braille reading times than
on a tactile display.  Furthermore, many vision impaired people still have
trouble reading. People with movement disorders may have perfectly good eye
sight, yet still be unable to track well enough to read text on a page.
