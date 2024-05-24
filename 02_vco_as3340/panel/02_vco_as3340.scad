width = 100;
include <kosmo.scad>


function col(n) = 18 + n*27.5;

 difference() {
     panel(width);
     
     jack(col(2), row(0)); // 1V/O In
     pot(col(1), row(0)); // Coarse
     pot(col(0), row(0)); // Fine
     
     jack(col(2), row(1)); // FM include
     pot(col(1), row(1)); // FM lvl
     jack(col(0), row(1)); // 1V/O Thru
     
     jack(col(2), row(2)); // HS+
     jack(col(1), row(2)); // HS-
     jack(col(0), row(2)); // SS
     
     jack(col(2), row(3)); // PWM in
     pot(col(1), row(3)); // PWM lvl
     pot(col(0), row(3)); // PWM offset
     
     jack(col(2), row(4)); // SQUARE out
     jack(col(1), row(4)); // SAW out
     jack(col(0), row(4)); // TRIANGLE out
     jack(col(2), row(5)); // SINE out
     switch(col(0), row(5)); // FREQ RANGE switch
     
     
     
 }
pcbHolders();