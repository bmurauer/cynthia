width = 50;
include <kosmo.scad>
n_rows = 6;

 difference() {
     panel(width);
     
    
     
     pot(col(1), row(0)); // A0
     pot(col(0), row(0)); // R0
     
     jack(col(1), row(1)); // GATE0
     jack(col(0), row(1)); // CV0
     
     jack(col(1), row(2)); // SIG IN 
     jack(col(0), row(2)); // SIG OUT
//        
     
     switch(col(1), row(3)); // pluck/AR0
     switch(col(0), row(3)); // pluck/AR1
       
     pot(col(1), row(4)); // A1
     pot(col(0), row(4)); // R1
     
     jack(col(1), row(5)); // GATE1
     jack(col(0), row(5)); // CV1
     
     
     
 }
pcbHolders(90);