include <eurorack.scad>

width = hp(6);


 difference() {
     panel(width);
     euroRackMountHole(15, 3, slot=3);
     euroRackMountHole(15, panelHeight - 3, slot=3);
     
     jack(9.5, 24.25);
     jack(20.5, 41.25);
     jack(9.5, 58.25);
     
     jack(15, 82.25);
     jack(15, 93.25);
     jack(15, 104.25);
     
 }