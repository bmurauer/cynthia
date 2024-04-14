width = 100;
include <kosmo.scad>

difference(){
    panel(width);
    midiSocket(width/2, panelHeight-35);
    
    buffer=20;

    jack(buffer, 40); 
    switch(width/2, 40);
    jack(width-buffer, 40);
    
    jack(buffer, 70); 
    switch(width/2, 70);
    jack(width-buffer, 70);
    
    jack(buffer, 100); 
    switch(width/2, 100);
    jack(width-buffer, 100);
    
    jack(buffer, 130); 
    switch(width/2, 130);
    jack(width-buffer, 130);
    
}
pcbHolders();