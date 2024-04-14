# POLY midi 2 cv

This module converts DIN MIDI to CV for up to four simultaneous signals.
It uses an Arduino Nano and it's based on [midi2cv](https://github.com/elkayem/midi2cv) by Larry McGovern.

It is capable of playing up to four notes at the same time. It offers 

 - four buffered 1V/Oct channels, with individual trimmers for each channel
 - four buffered 0/+10V gate channels 
 - four freely assignable inputs (I have not yet implemented any logic)

Thereby, it will iterate over the four outputs n0-n3 in the following way:

```pseudo
index = 0;
currentNotes = [0, 0, 0, 0];
play(note){
  for (i=0; i<=4; i++){
    checkedIndex = (index+i) % 4;
    if (currentNotes[checkedIndex] == 0) {
      play(note, currentNotes[checkedIndex])
    }
  }
  // all current gates are full, kick out the last 
  play(note, currentNotes[index]);
  index++;
}
```

At the time of writing this, I still have to evaluate this strategy in a more elaborate setup. However, this code is easily replaceable.
The same goes for the assignable inputs.
Some ideas that might be useful for those inputs are:
 - enable/disable channels
 - switch through different next-note selection strategies (like the jumper switch in [the original](https://github.com/elkayem/midi2cv))

 Of course, this module requires 4 of everything downstream, according to your taste (VCO, VCA, env gen, ...).
