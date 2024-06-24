width = 50;
include <kosmo.scad>
n_rows = 6;
colsEnd = 30;

 difference() {
     panel(width);
     
     jack(col(1), row(0)); // gate in
     jack(col(0), row(0)); // Output
     
     jack(col(1), row(1)); // Accent CV
     pot(col(0), row(1)); // Accent CV lvl
     
     jack(col(1), row(2)); // Pitch CV
     pot(col(0), row(2)); // Pitch CV lvl
     
     pot(col(1), row(3)); // Pitch
     pot(col(0), row(3)); // Distortion
     
     pot(col(1), row(4)); // Tune decay
     pot(col(0), row(4)); // Tune depth
     
     pot(col(1), row(5)); // Decay
     pot(col(0), row(5)); // Tone     
 }
pcbHolders();