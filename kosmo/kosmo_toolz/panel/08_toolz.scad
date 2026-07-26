width = 25;
include <kosmo.scad>
colPaddingLeft = 0;
colPaddingRight = 0;
n_rows = 6;
n_cols = 1;

 difference() {
     panel(width);
      
    
     
     jack( 12.5, 50); // Gate in
     
     switch( 12.5, 70); // INV 
     switch( 12.5, 85); // LIN/LOG
     
     pot( 12.5, 100); // OFFSET
     
     pot( 12.5, 125); // SLEW RATE
     
     jack( 12.5, 150); // OUT     
     
 }
pcbHolders();