# POLY midi 2 cv

This module converts DIN MIDI to CV for up to four simultaneous signals.
It uses an Arduino Nano and it's based on [midi2cv](https://github.com/elkayem/midi2cv) by Larry McGovern.

It is capable of playing up to four notes at the same time. It offers 

 - four buffered 1V/Oct channels, with individual trimmers for each channel
 - four buffered 0/+10V gate channels 
 - four switches to enable/disable channels

 Of course, this module requires 4 of everything downstream, according to your taste (VCO, VCA, env gen, ...).


## Voltage

This module requires a +5V supply from the 16-pin Eurorack header.
Alternatively, you may want to use a LM7805 to power the arduino.
