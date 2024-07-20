# AS3340-based VCO: "Oh, Silly!"

This design is based on the [design from Eddy Bergman](https://www.eddybergman.com/2020/01/synthesizer-build-part-18-really-good.html), which is based on a DigiSound 80 VCO.

I have made the following modifications to it:

 - I have included a switch that changes the 1nF capacitor to a 10nF one, allowing for mad slow LFO action.
 - the output stages are modified so that the VCO roughly produces -5V/+5V outputs (instead of 0/+10V).
 - the design includes a sine wave output. This is based on Eddy Bergmans Triangle to Sine converter.
 - the triangle wave has a dedicated centering trimmer, as I thought that centering the signal is essential for the sine wave conversion - I think if you measure your resistors a bit, you should be able to do without the trimmer.
 - the PWM input stage uses one less op amp. For the PWM signal, the polarity of the CV signal is irrelevant, as (1) there is no fixed notion what "positive PWM" looks like in the output wave, and (2) the two different resulting options sound the same to the human ear. If you have a certain setup for LFOs in mind where this detail is important, note that the signal may be inverted before being put into the AS3340 chip.
 - the design does not include a zener diode, as the square wave is centered and scaled like the other stages.
 - I removed the second 1V/O CV input, as I personally don't to use it. The FM input is plenty for me.

