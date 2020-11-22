#include "fsx.h"
  

const char* PROGMEM esphostname = "reinASgalerij";
const char* PROGMEM sourcefile = __FILE__;
const char* PROGMEM compile_time = __DATE__ " " __TIME__;

SemaphoreHandle_t fsSemaphore;

//---------------------------------------------------------------------------
int make_dirtree( fs::FS &ff, String path ){

  ff.mkdir( path );

  String fname = path + "/" + "een.txt";
  File file = ff.open(fname, FILE_WRITE);
  file.close(); 
  
 fname = path + "/" + "twee.txt";
  file = ff.open(fname, FILE_WRITE);
  file.close(); 

  fname = path + "/" + "drie.txt";
  file = ff.open(fname, FILE_WRITE);
  file.close(); 

  path = path + "/" + "sub1";

  Serial.printf( "Making directory %s , result : %d \n", path.c_str(), ff.mkdir( path  ) );
  String sub1path = path;
  
  fname = path + "/" + "vier.txt";
  file = ff.open(fname, FILE_WRITE);
  file.close(); 
  
  path = path + "/" + "sub2";
  Serial.printf( "Making directory %s , result : %d \n", path.c_str(), ff.mkdir( path  ) );
  
  fname = path + "/" + "vijf.txt";
  file = ff.open(fname, FILE_WRITE);
  file.close(); 

  path = path + "/" + "sub3";
  Serial.printf( "Making directory %s , result : %d \n", path.c_str(), ff.mkdir( path  ) );
  String sub3path = path;
   
  fname = path + "/" + "zes.txt";
  file = ff.open(fname, FILE_WRITE);
  file.close(); 

  
  String newpath = sub1path + "/sub3";
  Serial.println( "Result of rename "+ sub3path + " to " + newpath + " " +  String( ff.rename( sub3path, newpath ) )  ); 

}

//---------------------------------------------------------------------------


void setup(void) {

   enableCore0WDT(); 
   enableCore1WDT();

  Serial.begin(115200);
  Serial.print("\n");
  Serial.setDebugOutput(true);
  Serial.printf( "Sourcefile: %s\n",sourcefile );
  Serial.printf( "Compile time: %s\n",compile_time );

  updateSemaphore = xSemaphoreCreateMutex();;
  xSemaphoreTake(updateSemaphore, 10);
  xSemaphoreGive(updateSemaphore);

  tftSemaphore = xSemaphoreCreateMutex();;
  xSemaphoreTake(tftSemaphore, 10);
  xSemaphoreGive(tftSemaphore);

  
  mount_fs(); 
  startTFT();
  startWiFi();
  
}

void loop(void) {
  delay(5000);
 
  xSemaphoreTake( updateSemaphore, portMAX_DELAY);
  xSemaphoreGive( updateSemaphore);

}
