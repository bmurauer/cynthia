width = hp(4);
include <kosmo.scad>
n_rows = 6; 
START = 10;

 difference() {
     panelER(width);
     
     jack35(width / 2, START + 8 ); // GATE
     switch(width / 2, START + 19); // pluck/attack
     potSm(width / 2, START + 32); // ATTACK
     potSm(width / 2, START + 46); // RELEASE
     jack35(width / 2, START + 59); // ENV OUT
     jack35(width / 2, START + 85); // SIGNAL IN
     jack35(width / 2, START + 100); // SIGNAL OUT
     
     euroRackMountHole(10, 3.0, slot=3);
     euroRackMountHole(10, 128.5-3.0, slot=3);
 }