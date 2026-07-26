width = 75;
include <kosmo.scad>
n_rows = 6;

 difference() {
     panel(width);
     
     jack(col(1), row(0)); // gate in
     
     jack(col(2), row(1)); // Accent CV
     pot(col(1), row(1)); // Accent CV lvl
     
     jack(col(2), row(2)); // Tune CV
     pot(col(1), row(2)); // Tune CV lvl
     pot(col(0), row(2)); // Tune offset
     
     jack(col(2), row(3)); // Decay CV
     pot(col(1), row(3)); // Decay CV lvl
     pot(col(0), row(3)); // Decay offset
     
     pot(col(1), row(4)); // Tone
     
     
     jack(col(1), row(5)); // Output
     
     
     
     
 }
pcbHolders();