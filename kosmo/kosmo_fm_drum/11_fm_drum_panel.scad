width = 75;
include <kosmo.scad>


// function col(n) = 18 + n*27.5;

 difference() {
     panel(width);
     
     
     pot(col(0), row(0)); // TUNE OFFSET 1
     pot(col(1), row(0)); // TUNE OFFSET 2
     pot(col(2), row(0)); // DECAY OFFSET
     
     jack(col(0), row(1)); // TUNE CV
     pot(col(1), row(1)); // TUNE CV LVL
   
     pot(col(0), row(2)); // TUNE DEPTH
     pot(col(1), row(2)); // TUNE DECAY    
     
   
     
     jack(col(0), row(3)); // ACCENT CV
     jack(col(1), row(3)); // DECAY CV
     

     switch(col(2), row(1.5)); // FM ON/OFF
     switch(col(2), row(2.5)); // SINE/PULSE

     jack(col(0), row(4.5)); // GATE in
     jack(col(1.6), row(4.5)); // AUDIO out
     
     
     
 }
pcbHolders(90);