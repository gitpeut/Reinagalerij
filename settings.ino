#include "fsx.h"

struct appconfig  settings;


/*-----------------------------------------------*/
bool write_config()
{
fs::FS  *fs = fsdlist.back().f;
File    out=fs->open( "/settings.bin", "w");
const uint8_t   *b;
size_t  r,rt, size = sizeof( struct appconfig );

if ( !out ) return false;

b = ( uint8_t *)&settings;
for ( r= 0, rt=0; rt < size; rt += r ){
        r = out.write( b,size - rt);
        b += r;
}
out.close();
return true;
}


/*-----------------------------------------------*/
bool read_config()
{
fs::FS  *fs = fsdlist.back().f;
uint8_t  *b = (uint8_t *) &settings;
int r, rt;

File in = fs->open("/settings.bin", "r");
if (!in) {
  return( false );
}
int s = in.size();

for ( b = (uint8_t *)&settings, r=0,rt=0; rt < s; rt +=r ){ 
    r = in.read( b, s - rt );
    b += r;
}

in.close();
return true;
}

/*---------------------------------------------------*/
void show_settings(){
  
  Serial.printf( settings_format, settings.portland, settings.pauseseconds, settings.adapt_rotation, settings.portrait, settings.landscape);
  
}
/*---------------------------------------------------*/
void init_config(){

if ( !read_config()  ){  
  Serial.printf("couldn't open settings,use defaults\n");
  settings.portland = 0;
  settings.pauseseconds = 5;
  settings.adapt_rotation = 1;
  settings.portrait=0;//0 or 2
  settings.landscape=1; //1 or 3

  write_config();
}  

show_settings();
}
