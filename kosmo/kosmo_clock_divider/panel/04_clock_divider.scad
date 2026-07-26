width = 75;
include <kosmo.scad>

n_rows = 6;

difference(){
    panel(width);

    rotarySwitch(55, row(1.5)); // base division
    jack(55, row(3)); // gate
    jack(55, row(4)); // reset
    pushButton(55, row(5)); // reset switch
    
    // outputs
    for ( i = [0 : (n_rows-1)] ){
        jack(15, row(i));
        led(30, row(i));
    }
    
}
pcbHolders();