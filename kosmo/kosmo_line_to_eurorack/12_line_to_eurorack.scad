width = 50;
include <kosmo.scad>


// function col(n) = 18 + n*27.5;

 difference() {
     panel(width);
     
     jack(col(0), row(1)); // 1 in
     jack(col(1), row(1)); // 1 out
     jack(col(0), row(2)); // 2 in
     jack(col(1), row(2)); // 2 out
     jack(col(0), row(4)); // 3 in
     jack(col(1), row(4)); // 3 out
     jack(col(0), row(3)); // 4 in
     jack(col(1), row(3)); // 4 out

     
     
 }
pcbHoldersStripboard();