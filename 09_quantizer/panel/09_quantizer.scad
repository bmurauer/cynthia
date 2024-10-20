width = 50;
include <kosmo.scad>
n_rows = 6;
//colPaddingLeft=13.5;

 difference() {
     panel(width);
     
    
     
     sevenSegmentDisplay(25, row(0)); // base
     jack(width -13, row(1));
     pot(13, row(1));
     
     sevenSegmentDisplay(25, row(2)); // scale
     jack(width -13, row(3));
     pot(13, row(3));
     
     jack(col(1), row(4));
     jack(col(0), row(4));
     jack(col(1), row(5));
     jack(col(0), row(5));
     
     
 }
pcbHolders();