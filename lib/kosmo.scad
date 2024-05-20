panelHeight = 197;
thickness = 2.5;
mountHoleDiameter = 3.0;
jackHoleDiameter = 9.1;
ledHoleDiameter = 5;
pushButtonHoleDiameter = 7;
potHoleDiameter = 7;
switchHoleDiameter = 6;
rotarySwitchHoleDiameter = 9.1;
midiSocketHoleDiameter = 15;

offsetTopBottom = 20; // Mounting area

n_rows = 6;
n_cols = width / 25;

rowsStart = 30;
rowsEnd = panelHeight - 30;

colsStart = 15;
colsEnd = width - 15;


function row (i) = 30 + i * (rowsEnd - rowsStart) / (n_rows - 1);
function col (i) = 15 + i * (colsEnd - colsStart) / (n_cols - 1);

module pcbHolders(){
  pcbHolder(width-7, panelHeight/2 + 140/2 - 5);
  pcbHolder(width-7, panelHeight/2 - 140/2 - 5);
}


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

module pot(x, y) {
  translate([x, y , 0]) {
    union() {
      punchHole(0, 0, potHoleDiameter);
      translate([7.25, -2.75/2, -0.1]) {
        cube([1.5, 2.75, 1.6]);
      }
    }
  }
  echo(concat("pot at ", x, y));
}

module jack(x, y){
  punchHole(x, y, jackHoleDiameter);
    echo(concat("Jack at", x, y));
}

module panel(width) {
    
  echo(concat(n_rows, "rows, distance=",n_rows, (rowsEnd - rowsStart) / (n_rows-1)));
  echo(concat(n_cols, "cols, distance=",n_cols, (colsEnd - colsStart) / (n_cols-1)));
  difference() {
    cube([width, panelHeight, thickness]);
    kosmoMountHoles(width, panelHeight);
  }
}

