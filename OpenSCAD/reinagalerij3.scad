
//screen
visiblel    = 41;
visiblew    = 31;

visibleconnecto = 4.4; // offset from physicaledge at the connectorside
visibletopo     = 1.6;

physicall   = 49.2+0.2;
physicalw   = 35.7+0.2;
physicalh   = 2.3;

physicalconnecto = 6.5; // offset from the screen where the screen connector is to the side of the pcs
physicaltopo     = 5.6; // offsetfrom the side with the pcb connector to the side of the screen
physicalsideo    = 1.5; // actually 1.5 and 1.8

pcbl        = 60.6 +0.2;
pcbw        = 37.6 +0.2;
pcbh        = 1.6  +0.2;

conl        = 2.5;
conw        = 22+1;
conh        = 12.8 - pcbh;
cono        = 8.0;

//switch
swknobd     = 5.4 + 0.2;
swknobh     = 2.5;
swringd     = 7.7 + 0.2;
swringh     = 2.2;
swl         = 18.0;
swminl      = 12.3;
swminw      =  5.2;
sww         = 12.2;
swh         = 4.5;

//esp
esppcbl        = 51;
esppcbw        = 28;
esppcbh        = 1.6; 

espantl        = 6.8;
espantw        = 18+0.2;
espanth        = 1;
espanto        = 4.9;

espl           = 18.7;
esph           = 4.9 - esppcbh;

espsdl         = 3.4;
espsdw         = 14.8;
espsdh         = 1.9;
espsdo         = 6.6;

esptopcomph    = 9.9 - esppcbh;
espbotcomph    = 2.0;

//lipo
lipow       = 19.5;
lipol       = 40;
lipoh       = 7.8;

//usb

usbl        = 14.1+0.2;
usbw        = 15.0+0.2;
usbconl     = 7.5+0.2;
usbconw     = 5.5;
usbcone     = 1.0; // sticks 1 mm out over the edge
usbpcbh     = 1.4;
usbconh     = 4.0 - usbpcbh + 0.3;
usbcono     = 3.4 - 0.1;

cushion     = 5;

ww      = 2;
h       = 23;
w       = 70;
l       = 70;
r       = 3;
clickd  = 1.2; 

module usb(){
    color("green")
    cube([ usbl, usbw, usbpcbh]);
    
    color("grey")
    translate([ usbcono, usbw - usbconw + usbcone, usbpcbh])
        cube([ usbconl, usbconw, usbconh]);
    
}    

module lipo(){
    color("purple", 0.6)    
        cube([ lipol, lipow, lipoh] );    
}    


module esp(){
    
    color("grey")
    translate([ esppcbl, espsdo,espbotcomph - espsdh])
        cube([ espsdl, espsdw, espsdh]);   

    color("yellow",0.3)
        cube([ esppcbl, esppcbw, espbotcomph]);
    
    
    //pcb
    color("green")
    translate([0,0,espbotcomph])
        cube([ esppcbl, esppcbw, esppcbh]);   
    
    //top components
    color("yellow",0.3)
       translate([0,0,espbotcomph + esppcbh])
        cube([ esppcbl, esppcbw, esptopcomph]);
    // antenna
    color("red",0.7)
       translate([ -espantl, espanto,espbotcomph + esppcbh])
            cube([ espantl, espantw, espanth] );
    //esp32
    color("white",0.9)
       translate([ 0, espanto,espbotcomph + esppcbh])
            cube([ espl, espantw, esph] );
    
    
    
}    


module screen(){
    color("blue",0.2)
    cube( [visiblel, visiblew, 10] );
    
    translate( [-visibleconnecto, -(physicalw-visiblew)/2, 10] )
        color("white")
        cube( [physicall, physicalw, physicalh ]);
    
    
    translate( [-visibleconnecto - physicalconnecto, -(physicalw-visiblew)/2-(pcbw-physicalw)/2, 10 + physicalh] )
        color("green",0.8)
        cube( [pcbl, pcbw, pcbh + 1]);
   


    translate( [-visibleconnecto - physicalconnecto + pcbl - conl , -(physicalw-visiblew)/2-(pcbw-physicalw)/2 + cono, 10 + physicalh + pcbh] )
        cube([ conl, conw,conh]);
    
     
    translate( [-visibleconnecto - physicalconnecto, -(physicalw-visiblew)/2-(pcbw-physicalw)/2, 10 + physicalh +pcbh+1] )
        color("blue",0.1)
        cube( [pcbl, pcbw, pcbh + cushion]);
        
}    

module switch(){
    
        color("blue", 0.2 )
            cylinder( d= swknobd, h=swknobh);
        
        color("red")
            translate([0,0,swknobh])
                cylinder( d= swringd, h=swringh, $fn=64);
    
        color("black",0.9)
        translate([0,0,swknobh + swringh + swh/2])
            cube([ swl, sww,swh], center=true);
      
        color("blue",0.1)        
        translate([0,0,swknobh + swringh + swh/2 + swh])
            cube([ swl, sww, cushion + 6], center=true);
    
}



module box(){
  translate([ r, r,r]){      
    difference(){
    
    //cube( [l+2*ww,w+2*ww,h+2*ww ]); 
      
        hull(){    
        translate( [0,0,0] )
            sphere( r );   
        translate( [l,0,0] )
            sphere( r );    
        translate( [l,w, 0])
            sphere( r );    
        translate( [0,w,0 ])
            sphere( r );
    
        translate( [0,0,h] )
            sphere( r );   
        translate( [l,0,h] )
            sphere( r );    
        translate( [l,w, h])
            sphere( r );    
        translate( [0,w,h] )
            sphere( r );
       }    

       //clicks only on the l sides
       
       translate([ r,  w, h - r ])
        rotate([0,90,0])
         cylinder(d=clickd,h=l-2*r,$fn=12);
     
    
       translate([ r,0, h - r ])
        rotate([0,90,0])
         cylinder(d=clickd,h=l-2*r,$fn=12);
      
       //cut off top 
      translate( [-r,-r,h])
        cube( [ l+2*r, w+2*r, r] );               

    
        //  innerbox thicker bottom
        translate( [0,0,pcbh*2 + cushion] ) 
            cube( [l,w,h+ww] ); // +ww to h to make a hole
    
       
    translate( [ (l - (pcbl/2))/2 - r -r/2, (w-pcbw/2)/2 -r - r/2, -10 + physicalh -r] )
    screen();
   
    translate( [ l/2, sww/2, -swknobh -r] )
        switch();
    
    translate( [  l/2 -2*r, w - esppcbl, physicalh + pcbh + 2 ] )
    translate([ esppcbw,0,0] )
        rotate( [ 0,0,90 ])
            esp();

    translate( [0, 0, pcbh*2 + 2 -r] )
    translate([ lipow,0,0] )
        rotate( [ 0,0,90 ])
            lipo();

    translate([ r , w +r- usbw -usbcone/2,pcbh*2-usbpcbh +cushion])
        usb();

    }

}  
}
 

module lid(){
 
    translate([ r, r,r]){  
        difference(){
            
           hull(){    
            translate( [0,0,0] )
                sphere( r );   
            translate( [l,0,0] )
                sphere( r );    
            translate( [l,w, 0])
                sphere( r );    
            translate( [0,w,0 ])
                sphere( r );
           }
           
           //cut off top 
          translate( [-r,-r, 0])
            cube( [ l+2*r, w+2*r, r] );               
         

       }
       
       //inner rim
       difference(){ 
            cube([l,w,r*3]); 
               
            translate( [r/2,r/2,0 ] )cube([l-r,w-r,r*3]); 
               
           
 
        
      }
            //clicks only on the l sides
           
       translate([ 2*r,  w, r ])
        rotate([0,90,0])
         cylinder(d=clickd,h=l-4*r,$fn=12);
         
        
       translate([ 2*r,0, r ])
        rotate([0,90,0])
         cylinder(d=clickd,h=l-4*r,$fn=12);
  }
}    


box();
translate([l +10,0,0])
    lid();


/*
      translate( [ (l - (pcbl/2))/2 -ww/2, (w-pcbw/2)/2 -ww , -10 + physicalh] )
    screen();
   
    translate( [ ww +l/2, sww -ww, -swknobh ] )
        switch();
    
    translate( [  l/2-2*ww, ww + w - ( esppcbl), ww+pcbh*2 + 2] )
    translate([ esppcbw,0,0] )
        rotate( [ 0,0,90 ])
            esp();

    translate( [ww+1, ww , ww+pcbh*2 + 2] )
    translate([ lipow,0,0] )
        rotate( [ 0,0,90 ])
            lipo();

    translate([ 2*ww, 2*ww + w - usbw -usbcone/2,ww+pcbh*2-usbpcbh +cushion])
        usb();
*/