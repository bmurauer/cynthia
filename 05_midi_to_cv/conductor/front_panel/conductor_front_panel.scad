include <eurorack.scad>

width = hp(6);


 difference() {
     panel(width);
     euroRackMountHole(15, 3, slot=3);
     euroRackMountHole(15, panelHeight - 3, slot=3);
 }