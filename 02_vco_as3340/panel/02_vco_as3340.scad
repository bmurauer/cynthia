width = 75;
include <kosmo.scad>


// function col(n) = 18 + n*27.5;

 difference() {
     panel(width);
     
     jack(col(0), row(0)); // 1V/O in
     pot(col(1), row(0)); // coarse
     pot(col(2), row(0)); // fine

     
     jack(col(0), row(1)); // FM in
     pot(col(1), row(1)); // FM lvl
     jack(col(2), row(1)); // 1V/O Thru
     
     jack(col(0), row(2)); // SYNC in
     rotarySwitch(col(1), row(2)); // SYNC mode
     switch(col(2), row(2)); // FREQ RANGE switch
     
     jack(col(0), row(3)); // PWM CV in
     pot(col(1), row(3)); // PWM CV lvl
     pot(col(2), row(3)); // PWM offset
     
     jack(col(1.5), row(4)); // SQUARE out
     jack(col(0.5), row(4)); // SAW out
     jack(col(1.5), row(5)); // TRIANGLE out
     jack(col(0.5), row(5)); // SINE out
     
     
     
 }
pcbHoldersStripboard();