panelHeight = 197;
panelHeightER = 128.5;
thickness = 2;
mountHoleDiameter = 3.0;
jackHoleDiameter = 9.1;
jack35HoleDiameter = 6;
ledHoleDiameter = 5;
pushButtonHoleDiameter = 7;
potHoleDiameter = 7;
switchHoleDiameter = 6;
rotarySwitchHoleDiameter = 9.1;
midiSocketHoleDiameter = 15;


itemWidth = 20;
itemHeight = 25;

rowsStart = 25; // offset for frame 
colsStart = 10; // offset for PCB holders

function row (i) = rowsStart + i * itemHeight + (itemHeight / 2);
function col (i) = width - (colsStart + i * itemWidth + (itemWidth / 2));

module pcbHoldersStripboard(){
  pcbHolders(27*2.54);
}

module pcbHolders(distance) {
  pcbHolder(width-5, panelHeight/2 + distance/2 - 5);
  pcbHolder(width-5, panelHeight/2 - distance/2 - 5);
}

function hp(x) = x * 5.08 - 0.3;

module pcbHolder(x, y){
  pcbHole = 3;
  w = 10;
  h = 20;
  clearance = 15; // distance between panel and PCB hole
    
  translate([x, y, -0.1]) { // prevent gap
    rotate([0, 90, 0]) {
      union() {
        difference() {
          translate([0, 0, -5]) {               
            cube([5,w,5]);
          }
          translate([5, w+1, -5]) {
            rotate([90, 0, 0]) {
              cylinder(r=5, h=w+2,$fn=49);
            }
          }
        }
        /**/
        intersection() {
          difference() {
            cube([h, w, thickness+1]);
            translate([clearance, w/2, -0.9]){
              cylinder(r=pcbHole/2,  h=thickness+2, $fn=20);
              cylinder(r=3.1, h=2.3, $fn=6);
            }
          }
          translate([h/2-2, w/2, -1]) {
            cylinder(r=w+1, h=thickness+2, $fn=40);
          }
        }
        /**/
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

// Eurorack mounting hole.
//   x, y      : hole center position (panel coordinates)
//   d         : hole diameter (3.2 default = M3 clearance, slightly oversize)
//   slot      : horizontal slot length for tolerance (0 = plain round hole)
//   depth     : cut depth; leave default to punch fully through any panel thickness
module euroRackMountHole(x, y, d = 3.2, slot = 0, depth = 100) {
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

module midiSocket(x, y) {
    punchHole(x, y, midiSocketHoleDiameter);
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
  translate([x, y , 0]) {
    union() {
      punchHole(0, 0, potHoleDiameter);
      translate([7.25, -2.75/2, -0.1]) {
        cube([1.5, 2.75, 1.6]);
      }
    }
  }
}

module potSm(x, y) {
  translate([x, y , 0]) {
    punchHole(0, 0, potHoleDiameter);
  }
}

module jack(x, y){
  punchHole(x, y, jackHoleDiameter);
    echo(concat("Jack at", x, y));
}
module jack35(x, y){
  punchHole(x, y, jack35HoleDiameter);
    echo(concat("Jack at", x, y));
}


module panel(width) {
  difference() {
    cube([width, panelHeight, thickness]);
    kosmoMountHoles(width, panelHeight);
  }
}
module panelER(width) {
   cube([width, panelHeightER, thickness]);
}


