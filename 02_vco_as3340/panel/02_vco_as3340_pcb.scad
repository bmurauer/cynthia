width = 100;
include </home/benjamin/git/cynthia/lib/openscad/kosmo.scad>


conX = 50;
conY = 90;

start = conX - 29.114;
offset = 18.2726;
top = conY - 23.622;
btm = conY + 49.928;

mirror([0, panelHeight/2, 0]){    
    difference() {
        panel(width);

        
        jack(start + 0 * offset, top); // 1V/O in
        jack(start + 1 * offset, top); // PWM CV
        jack(start + 2 * offset, top); // PWM CV
        jack(start + 3 * offset, top); // PWM CV
        
        // sub board
        translate([conX, conY, 0]) {
            pot( - 18.142,  -6.522); // Coarse
            pot( 15.386,  -6.522); // Fine
            
            pot(- 18.142, 10.496); // PWM Offset
            pot( - 1.378,  10.496); // PWM LVL
            pot( 15.386,  10.496); // PWM LVL
         
            switch( - 19.666,  30.894); // mode
            rotarySwitch( -1.378,  31.608); // sync mode
            
        }

        
        jack(start + 0 * offset, btm); // 1V/O in
        jack(start + 1 * offset, btm); // PWM CV
        jack(start + 2 * offset, btm); // PWM CV
        jack(start + 3 * offset, btm); // PWM CV   
    }
}

 /*
 difference() {
     



     
     

     
     
 }
*/