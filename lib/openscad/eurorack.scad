panelHeight = 128.5;
thickness = 2;
mountHoleDiameter = 3.0;
jackHoleDiameter = 6;
ledHoleDiameter = 5;
pushButtonHoleDiameter = 7;
potHoleDiameter = 6;
switchHoleDiameter = 6;
rotarySwitchHoleDiameter = 9.1;


function hp(x) = x * 5.08 - 0.3;


// Eurorack mounting hole.
//   x, y      : hole center position (panel coordinates)
//   d         : hole diameter (3.2 default = M3 clearance, slightly oversize)
//   slot      : horizontal slot length for tolerance (0 = plain round hole)
//   depth     : cut depth; leave default to punch fully through any panel thickness
module euroRackMountHole(x, y, d = 3.2, slot = 1, depth = 100) {
    translate([x, y, -depth/2])
        hull() {
            // two cylinders spaced by `slot` make an obround (stadium) slot;
            // slot = 0 collapses to a single round hole
            for (dx = [-slot/2, slot/2])
                translate([dx, 0, 0])
                    cylinder(h = depth, d = d, $fn = 32);
        }
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

module led(x, y) {
    punchHole(x, y, ledHoleDiameter);
}
module switch(x, y) { 
    punchHole(x, y, switchHoleDiameter);
}
module pushButton(x, y) { 
    punchHole(x, y, pushButtonHoleDiameter);
}
module rotarySwitch(x, y) { 
    punchHole(x, y, rotarySwitchHoleDiameter);
}
module sevenSegmentDisplay(x, y) {
    translate([x-6.5, y-9.6, -1]){
        cube([13, 19.2, thickness+2]);
    }
}

module pot(x, y) {
  punchHole(x, y, potHoleDiameter);
}

module jack(x, y){
  punchHole(x, y, jackHoleDiameter);
}

module panel(width) {
   cube([width, panelHeight, thickness]);
}

