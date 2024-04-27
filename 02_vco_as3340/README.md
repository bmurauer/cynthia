# AS3340-based VCO

This design is based on the [design from Eddy Bergman](https://www.eddybergman.com/2020/01/synthesizer-build-part-18-really-good.html), which is based on a DigiSound 80 VCO.

I have made the following modifications to it:

 - the output stages are modified so that the VCO roughly produces -5V/+5V outputs (instead of 0/+10V)
 - the design includes a sine wave output
 - the triangle wave has a dedicated centering trimmer, as centering the signal is essential for the sine wave conversion
 - the PWM input stage uses one less op amp
 - the design does not include a zener diode, as the square wave is centered and scaled like the other stages
 - I removed the second 1V/O CV input, as I don't seem to use it.

