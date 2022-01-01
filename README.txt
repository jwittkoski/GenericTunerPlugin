The Generic Tuner Plugin is a tuning plugin for SageTV.
It has only been tested on Linux, but may work on the other
platforms supported by SageTV with modifications.

The Generic Tuner Plugin was created to move the complexities
of tuning your myriad of devices from a poorly understood C
library to the language of your choice.

Basically, the plugin acts as a passthru to a script called
gentuner. gentuner can be written in whatever language you
desire. It accepts several arguments on the command line,
and returns results on stdout.

gentuner needs to do several things:

 - Report what remotes are available
 - Report what keys each remote has
 - Actually tune the device using the method of your choice

In addition, anything else you might want to do when
tuning a channel could be included. See the section about
gentuner.CUSTOM below for more details.

gentuner must recognize the following arguments:

REMOTES
    Action for gentuner to take:
        return a list (one per line) of available remotes
KEYS remote
    Action for gentuner to take:
        return a list (one per line) of keys that "remote" supports
SEND remote key
    Action for gentuner to take:
        transmit "key" to "remote"
        How exactly you transmit this key depends on your setup.
TUNE remote channel
    Action for gentuner to take:
        use "remote" to tune to "channel"
        "channel" is an integer
        How exactly you tune depends on your setup.
CAN_TUNE remote
    Action for gentuner to take:
        returns string "OK" if "remote" can tune by channel (i.e.
        macro tune) instead of using individual keys

Note that currently "remote", "key", and "channel" are strings
that are expected to have no whitespace in them. For example
"FiOS-TV-1" is a valid remote name, whereas "FiOS Remote 1" is not.
This limitation may be fixed in the future.

If gentuner returns OK for CAN_TUNE, Sage will ONLY call TUNE for
that remote - it will never call SEND. Generally this would be used
for FireWire tuners that can accept a full channel instead of
individual keypresses.

If gentuner returns anything except OK for CAN_TUNE, Sage will
ONLY call SEND for that remote - it will never call TUNE.
SEND will be called multiple times, typically once for each
digit of a channel, plus any pre or post keys you define within
Sage itself. Generally if you use a tuning method that deals
with key presses, like LIRC, this is the path for you.

A gentuner script can be simple or complex. A simple script might
be a thin translation layer to LIRC (for IR based tuning) or 6200ch
or panelctl (for FireWire tuning). See gentuner.LIRC for a simple
example.

A more complex script might report "OK" for CAN_TUNE, and then
sends each digit individually so that you can control the timing
between digits more precisely. Or you might want add code to do
something important for your setup before or after each tune.
See gentuner.CUSTOM for a more complex example.

There are example gentuner scripts included in this package. Not all
have been tested exhaustively.

