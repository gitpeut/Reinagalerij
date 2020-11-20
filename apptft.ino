#include "fsx.h"

#include "SPI.h"
#include <TFT_eSPI.h>   
#include <TJpg_Decoder.h>

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite img  = TFT_eSprite(&tft); 
  
TaskHandle_t  tftshowTask;
SemaphoreHandle_t tftSemaphore;

/*
 * #define TFT_BACKLIGHT_ON HIGH 
 * #define TFT_MOSI 23
 * #define TFT_SCLK 19  // was 18
 * #define TFT_CS    5  // from 15 to avoid sd card conflict Chip select control pin
 * #define TFT_DC    0  // from 2 to avoid sd card conflict Data Command control pin
 * #define TFT_RST  22  // Reset pin (could connect to RST pin)
 * #define TFT_BL   21 
 */
//---------------------------------------------------------------------
void tft_message( const char  *message1, const char *message2 ){

img.createSprite( tft.width(), 100);
img.setTextColor( TFT_WHITE, TFT_BLACK ); 
img.setTextSize(2);
img.fillSprite(TFT_BLACK);


img.drawString( message1, 0,10, 2);  
img.drawString( message2, 0,60, 2);  

 
xSemaphoreTake( updateSemaphore, portMAX_DELAY);  
  img.pushSprite( 0, tft.height()/2 - 50);
  delay( 10000 );
xSemaphoreGive( updateSemaphore); 

img.deleteSprite();
}

//---------------------------------------------------------------------
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
   // Stop further decoding as image is running off bottom of screen
  if ( y >= tft.height() ) return 0;

  // This function will clip the image block rendering automatically at the TFT boundaries
  tft.pushImage(x, y, w, h, bitmap);

  // This might work instead if you adapt the sketch to use the Adafruit_GFX library
  // tft.drawRGBBitmap(x, y, bitmap, w, h);

  // Return 1 to decode next block
  return 1;
}
//-----------------------------------------------------------------------
void fsjpgsize( fs::FS &fs, const char *filename, uint16_t *w, uint16_t *h){
  
fs::File jpgfile = fs.open( filename);
TJpgDec.getFsJpgSize(w,h, jpgfile);
// file should be closed by getFsJpgSize 
}
//-----------------------------------------------------------------------
void fsdrawjpg( fs::FS &fs, const char *filename, uint32_t x_offset, uint32_t y_offset  ){
  
fs::File jpgfile = fs.open( filename);
TJpgDec.drawFsJpg (x_offset, y_offset, jpgfile);
// file should be closed by drawFsJpg
}

//-----------------------------------------------------------------------
void showpicture( fs::FS &fs, const char *filename){

 uint16_t w = 0, h = 0, scale;
 uint32_t xo = 0, yo = 0;

 
 //TJpgDec.getFsJpgSize(&w, &h, filename); // Note name preceded with "/"
 
 fsjpgsize( fs, filename, &w, &h);
 
 if ( w==0 && h ==0 ) return;

 tft.setRotation( settings.portrait );
 if( settings.portland )tft.setRotation( settings.landscape );

 if ( settings.adapt_rotation ){
  if ( w > h ){
    if ( !settings.portland )tft.setRotation( settings.landscape);   
  }else{
    if ( settings.portland )tft.setRotation( settings.portrait);     
  }
 }
 
  for (scale = 1; scale <= 8; scale <<= 1) {
      //   if ( w < ((tft.width() * scale* 2)/3) && scale > 1 )scale>>=1;  
      //if ( w < ((tft.width() * scale* 2)/3) && scale > 1 ){
     if ( w < ((tft.width() * scale* 2)/3) && scale > 1 ){
        scale>>=1;           
        break;
    }
  }

 TJpgDec.setJpgScale(scale);
 
 // if picture is smaller, then center
 // if ( w < tft.width()*scale )  xo = (tft.width() - w/scale)/2;
 //if ( h < tft.height()*scale ) yo = (tft.height() - h/scale)/2;

  if ( w < tft.width()*scale )   xo = ( tft.width() - w/scale)/2;
  if ( h < tft.height()*scale )  yo = ( tft.height() - h/scale)/2;

 if ( xo || yo)tft.fillScreen(TFT_BLACK);
  
 Serial.printf("%s Width = %d,height %d xo = %d, yo = %d\n",filename, w/scale,h/scale, xo,yo); 

 // Draw the image, top left at xo, yo
 //TJpgDec.drawFsJpg(xo, yo, filename);
 
 xSemaphoreTake( updateSemaphore, portMAX_DELAY);
  fsdrawjpg( fs, filename, xo, yo );
 xSemaphoreGive( updateSemaphore); 

 delay( settings.pauseseconds * 1000 );
 
 return;
}
//-----------------------------------------------------------------------------
void traversefs( fs::FS &fs, const char *dirname ){ 

fs::File dir = fs.open( dirname );

if(!dir.isDirectory()){
    //Serial.println(" - not a directory");
    return;
}

fs::File entry;
  
while ( (entry = dir.openNextFile()) ){

    xSemaphoreTake( updateSemaphore, portMAX_DELAY);
    xSemaphoreGive( updateSemaphore);

    char *tmpname= strdup( entry.name() );
    if ( entry.isDirectory() ) {
        entry.close();
        traversefs( fs, tmpname );
   }else{
        entry.close(); 
        showpicture ( fs, tmpname );
   }
   free( tmpname );      
}

dir.close();

}

//-------------------------------------------------------


void initTFT(){

  init_config();
  
  tft.begin();
  tft.setTextColor(0xFFFF, 0x0000);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true); // We need to swap the colour bytes (endianess)
  tft.setRotation( settings.portrait );
  // The jpeg image can be scaled by a factor of 1, 2, 4, or 8
  TJpgDec.setJpgScale(1);

  // The decoder must be given the exact name of the rendering function above
  TJpgDec.setCallback(tft_output);

   showpicture ( *(fsdlist.back().f) , "/logo.jpg" );
  
}

//---------------------------------------------------------------------------------------
void runtftShow( void *param){
  
   
  initTFT();

  while(1){
    refresh_fsd_usage();
    for ( auto &efes : fsdlist ){
      Serial.printf ("Now traversing fs %s\n", efes.fsname);
      traversefs( *(efes.f), "/" );    
    }
  }  
}

//---------------------------------------------------------------------------------------

void startTFT(){

     xTaskCreatePinnedToCore( 
         runtftShow,                                      // Task to handle special functions.
         "TFTshower",                                  // name of task.
         1024*4,                                       // Stack size of task
         NULL,                                         // parameter of the task
         2,                                            // priority of the task
         &tftshowTask,                               // Task handle to keep track of created task 
         1);                                          //core to run it on

  
}
