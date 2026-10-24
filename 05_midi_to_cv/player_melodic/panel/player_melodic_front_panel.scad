include <eurorack.scad>

width = hp(4);
n_top = 0;
n_bot = 1;

 difference() {
     panel(width);
     
     euroRackMountHoles(n_top, n_bot);
       
     jack(10, 19.75);
     jack(10, 30.75);
     
     microswitch(13.5, 73.5);
     led((8.5+4)/2, 73.5);
     
     
     jack(10, 108.75);
     
     
 }