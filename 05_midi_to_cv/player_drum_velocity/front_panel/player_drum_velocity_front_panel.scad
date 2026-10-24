include <eurorack.scad>

 difference() {
     panel(hp(4));
     
     euroRackMountHoles(0, 1);
       
     jack(10, 19.75);
     jack(10, 30.75);
     
     microswitch((11.5+16)/2, 65.5);
     led((6.5-1.27), 65.5);
     
     jack(10, 97.75);
     jack(10, 108.75);     
 }