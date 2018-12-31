# A Generic Tuner Plugin for SageTV

## Description

The Generic Tuner Plugin is a tuning plugin for SageTV.

It was created to move the complexities of tuning your myriad of recording devices from a poorly understood C library to the language of your choice.

Currently it has only been tested on Linux, but may work on the other platforms supported by SageTV with modifications.

The plugin acts as a passthru to a script or program called `gentuner`. `gentuner` can be written in whatever language you desire. It needs to accept several simple arguments on the command line and return results on STDOUT. gentuner is responsible for handing off the actual tuning work to the appropriate software (LIRC, `6200ch`, `panelctl`, etc).

## gentuner

You must create a `gentuner` script that is appropriate for your setup.

There are several example `gentuner` scripts provided in the distribution. You might want to have one of them handy for reference as you read the following section.

`gentuner` needs to do several things:

* Report what "remotes" are available
* Report what "keys" each "remote" has
* Actually tune the recording device using the method of your choice

In addition, anything else you might want to do when tuning a channel could be included. See the section about `gentuner.CUSTOM` below for more details.

A "remote" need not be a physical remote. It merely refers to the device or mechanism used to tune the recording device to a specified channel.

`gentuner` must recognize the following command line arguments:

<table>
  <tr><td><strong>Command</strong></td><td><strong>Action for gentuner to take</strong></td></tr>
  <tr><td><tt>REMOTES</td><td>Return a list (one per line) of available "remotes"</td></tr>
  <tr><td><tt>KEYS <em>remote</em></tt></td><td>Return a list (one per line) of "keys" that <em>remote</em> supports</td></tr>
  <tr><td><tt>SEND <em>remote</em> <em>key</em></tt></td><td>Transmit <em>key</em> to <em>remote</em>. How exactly you transmit this "key" depends on your setup.</td></tr>
  <tr><td><tt>TUNE <em>remote</em> <em>channel</em></tt></td><td>Use <em>remote</em> to tune to <em>channel</em>. <em>channel</em> is an integer. How exactly you tune depends on your setup.</td></tr>
  <tr><td><tt>CAN_TUNE <em>remote</em></tt></td><td>Returns string "OK" if <em>remote</em> can tune by channel (i.e. macro tune) instead of using individual keys.</td></tr>
</table>

Note that currently _remote_, _key_, and _channel_ are strings that are expected to have no whitespace in them. For example "FiOS-TV-1" is a valid remote name, whereas "FiOS TV 1" is not. This limitation may be fixed in the future.

### Tuning methods
The plugin supports two methods for tuning:
 * Macro Tuning - The plugin will call `gentuner` with the full channel number (i.e. `TUNE 123` for channel "123") and expect `gentuner` will do everything necessary tune the recording device to that channel.
 * Keypress - The plugin will issue a `SEND` command for each keypress (i.e. `SEND _remote_ 1` then `SEND _remote_ 2` then `SEND _remote_ 3` for channel "123") and expect that each keypress is sent to the recording device.

Before each channel tune, the plugin will call `gentuner` with the `CAN_TUNE` command.

If `gentuner` returns `OK` for `CAN_TUNE`, the plugin will *only* call `TUNE` ("macro tuning" method) for that remote - it will never call `SEND` ("keypress" method). Generally this would be used for a tuning method that doesn't have "keys" such as using FireWire to tune the channel on a set top box. It might also be used when you want `gentuner` to do all the tuning work. If a remote returns `OK` for `CAN_TUNE`, the list of "keys" returned by `KEYS` is ignored by the plugin since they are not needed.

If `gentuner` returns anything except `OK` for `CAN_TUNE`, the plugin will *only* call `SEND` for that remote - it will never call `TUNE`. `SEND` will be called multiple times, typically once for each digit of a channel, plus any pre or post keys you define within Sage itself. Generally if you use a tuning method that deals with key presses, like LIRC, this is the path for you.

The plugin remembers the answer to `CAN_TUNE` on a per remote basis, so some of your "remotes" could use the "keypress" method and some can use the "macro tune" method if necessary.

A `gentuner` script can be simple or complex. A simple script might be a thin translation layer to LIRC for IR based tuning (see `gentuner.LIRC` for an example) or `6200ch` or `panelctl` for FireWire tuning (see `gentuner.PANELCTL` for an example).

A more complex `gentuner` might report `OK` for `CAN_TUNE`, and then send each digit individually so that you can control the timing between digits more precisely. Or you might want add code to do something important for your setup before or after each tune. See `gentuner.CUSTOM` for a more complex example.

## Installation

### Building

#### For SageTV 9

SageTV 9 is released as 64 bit only, so the Makefile only supports building a 64 bit plugin.

You'll need `gcc` and `make` installed on your build host.

Once you have gcc installed, just:

    make

And you should have a `GenericTunerPlugin.so`.

#### For earlier versions of SageTV which require a 32 bit plugin

If you need to build a 32 bit version of the plugin for a SageTV version earlier than 9, change the `-m64` to `-m32` in the Makefile.

You'll need `gcc` and `make` installed on your build host.

If you are compiling on a 64 bit Ubuntu distribution you will need to install the gcc-multilib package to build 32 bit executables:

    apt-get install gcc-multilib

Once you have the build tools installed, just:

    make

And you should have a `GenericTunerPlugin.so`.

### Install the plugin

*IMPORTANT NOTE*: After shutting down the SageTV process, always create a backup copy of your Sage.properties file before making any changes! If you make any changes to Sage.properties that break your setup, you can always revert to the backup copy.

* Put the plugin in the right location

        cp GenericTunerPlugin.so /opt/sagetv/server/irtunerplugins/
 
* Examine the sample `gentuner.*` scripts to see which one most closely fits in with your tuning needs, and make a copy. For example:

        cp gentuner.LIRC gentuner.LOCAL

* Modify `gentuner.LOCAL` as needed for your environment and/or tuning needs.
* Test your version of `gentuner` with your setup directly from the command line to make sure it's tuning the devices as expected before trying it with Sage. You could try things like:

        gentuner.LOCAL REMOTES

  or

        gentuner.LOCAL KEYS REMOTE_NAME_HERE

  or

        gentuner.LOCAL CAN_TUNE REMOTE_NAME_HERE
    
  or

        gentuner.LOCAL SEND REMOTE_NAME_HERE 1
        gentuner.LOCAL SEND REMOTE_NAME_HERE 2
        gentuner.LOCAL SEND REMOTE_NAME_HERE 3
        gentuner.LOCAL SEND REMOTE_NAME_HERE OK
        
  or
  
        gentuner.LOCAL TUNE REMOTE_NAME_HERE 123
  
  etc.

* Copy your version of `gentuner` to where the plugin expects it. Note that currently the Generic Tuner Plugin has the path to `gentuner` hardcoded to `/opt/sagetv/server/gentuner`. You can create a symlink or recompile if you need to put the plugin elsewhere.

        cp gentuner.LOCAL /opt/sagetv/server/gentuner

* Configure each video source to use the Generic Tuner Plugin.
 Unfortunately, SageTV does not provide any way to change the tuning plugin for a video souce after the source is initially configured. You must remove and readd the video source, selecting "Generic Tuner" when asked to select a tuning plugin. *Note that removing and re-adding the video source will likely involve other work to get the configuration back to where you had it.*
* Stop and start SageTV
* In SageTV, go to the Setup -> Setup Video Sources menu, select the video source that uses the Generic Tuner, click on "Tuner Control", then "Change Tuning Device". You should see the list of the remotes your `gentuner` script reports. Select it.

Note: Currently debugging is enabled and lots of stuff will get logged to `/opt/sagetv/server/gentuner.log`. You can disable that in the source if desired. An option may be added later.

## History
The Generic Tuner Plugin is based on [MultiDCTTunerDLL](http://sourceforge.net/projects/multidcttuner/) which is Copyright (C) 2004 Sean Newman and was licensed under the GPLv2 license.

This code was written and/or modified by by Jim Paris, Frey Technologies LLC, Sean Newman, Sean Stuckless, and John P. Wittkoski.

Thanks to Jerry Fiddler for the `gentuner.PANELCTL` script and the changes he made to `panelctl` to support multiple STBs.

## Developer Details
_Tidbits about why the plugin does what it does will be added as time permits._
