defaultHeight = 195;
thickness = 2.5;
mountHoleDiameter = 3.0;
jackHoleDiameter = 9.1;
potHoleDiameter = 7;
switchHoleDiameter = 6;

buffer = 12.5;

row0 = 45;
row1 = 80;
row2 = 120;
row3 = 155;


module pcbHolders(){
  pcbHolder(width-5, defaultHeight/2 + 140/2 - 5);
  pcbHolder(width-5, defaultHeight/2 - 140/2 - 5);
}


module pcbHolder(x, y){
  pcbHole = 3;
  w = 10;
  h = 20;
  translate([x, y, 1]) {
    rotate([0, 90, 0]) {
      intersection() {
        difference() {
          cube([h, w, thickness+1]);
          translate([h-pcbHole-2, w/2, -1]){
            cylinder(r=pcbHole/2,  h=thickness+2, $fn=20);
            cylinder(r=6.2/2, h=2.3, $fn=6);
          }
        }
        translate([h/2-2, w/2, -1]) {
          cylinder(r=w+1, h=thickness+2, $fn=40);
        }
      }
    }
  }
}



module kosmoMountHoles(width, height) {
  punchHole(5, 5, mountHoleDiameter,true);
  punchHole((width-5), 5, mountHoleDiameter,true);
  punchHole(5, (height-5), mountHoleDiameter,true);
  punchHole((width-5), (height-5), mountHoleDiameter,true);
}


module punchHole(x, y, holeSize,chamfer=false){
  translate([x, y, -1]){
    cylinder(r=holeSize/2, h=thickness+2, $fn=20);
    if(chamfer) {
      translate([0, 0, 0.5]){
        cylinder(r2=holeSize, r1=0, h=holeSize+0.5, $fn=20);
      }
    }
  }
}

module pot(x, y) {
  translate([x, y , 0]) {
    union() {
      punchHole(0, 0, potHoleDiameter);
      translate([7.25, -2.75/2, -0.1]) {
        cube([1.5, 2.75, 1.6]);
      }
    }
  }
}

module jack(x, y){
  punchHole(x, y, jackHoleDiameter);
}

module panel(width) {
  difference() {
    cube([width, defaultHeight, thickness]);
    kosmoMountHoles(width, defaultHeight);
  }
}

