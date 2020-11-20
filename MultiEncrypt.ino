#include "fsx.h"

struct netp{
  uint8_t  ssid[48]; // max 32
  uint8_t  pass[64]; //max 63
};

TaskHandle_t      startwifiTask;   
std::vector<struct netp>  netpass;

uint8_t key[32];
uint8_t iv[16];

WiFiMulti wifiMulti;

fs::FS multifs = LITTLEFS;

//---------------------------------------------------------------------------------------

void WiFiLostIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
      stopFSXServer();
      tft_message( "Niet meer verbonden", "met een netwerk"); 
      return;
}
//---------------------------------------------------------------------------------------

void WiFiGotIP(WiFiEvent_t event, WiFiEventInfo_t info)
{
    Serial.println("WiFi connected");
    Serial.print("Obtained IP address: ");
    Serial.print(WiFi.localIP());
    Serial.print( " on WiFi network with SSID ");
    Serial.println(WiFi.SSID() );

    startFSXServer();
    
    bool foundssid = false; 
    for ( auto n : netpass ){
      if ( !strcmp( (char *)n.ssid, WiFi.SSID().c_str() ) ){
        // Maybe the pasword changed?
        if ( strcmp( (char *)n.pass, WiFi.psk().c_str() ) ){
             memset( n.pass, 0, sizeof( n.pass ) );
             strcpy( (char *)n.pass, WiFi.psk().c_str() );
        }else{
          foundssid = true;
        }
        break;  
      }
    }
    if ( !foundssid ) {
      struct netp n;
      
      strcpy( (char *)n.pass, WiFi.psk().c_str() );
      strcpy( (char *)n.ssid, WiFi.SSID().c_str() );
      
      netpass.push_back( n );
      netp2file();
    }else{
      Serial.println( "network already known");
    }
    //displaynetp();
    tft_message( "Goed. IP adres:", WiFi.localIP().toString().c_str() ); 


}

//---------------------------------------------------------------------------------------

void runWiFi( void *param){

  //deleteFile(multifs, "/netpass");

  file2netp();
  //displaynetp();
         
  WiFi.onEvent(WiFiGotIP, WiFiEvent_t::SYSTEM_EVENT_STA_GOT_IP); 
  WiFi.onEvent(WiFiLostIP, WiFiEvent_t::SYSTEM_EVENT_STA_LOST_IP); 
  
  WiFi.mode(WIFI_STA);

  Serial.println("Waiting for WiFi");
  
  for ( int i=0 ; i < 10; ++i){
    int mstatus;
    if ( (mstatus = wifiMulti.run()) == WL_CONNECTED) break;  
    Serial.printf( "%d mstatus = %d\n", i, mstatus);  
    if ( mstatus == 6) break;
    delay(2000);
  }
    
  if ( WiFi.status() != WL_CONNECTED  ){
    
    //Init WiFi as Station, start SmartConfig
     //WiFi.mode(WIFI_AP_STA);
    tft_message("Onbekend netwerk. Start ", "ESP Touch op telefoon" );  
    WiFi.beginSmartConfig();



    Serial.println("Started smartconfig");

    // 2 minutes to connect using smartconfig
    for (int i=0; !WiFi.smartConfigDone() && i < 120 ; ++i ) {
      delay(500);
      Serial.print(".");
    }
    
    if ( !WiFi.smartConfigDone() ){
      Serial.printf( "\n%s stop of smartconfig. No WiFi.", WiFi.stopSmartConfig()?"Successful":"Failed");
    }
      
  }

  delay(100);
  netpass.clear();
  netpass.shrink_to_fit();
  vTaskDelete( NULL );
 
}
//---------------------------------------------------------------------------------------

void startWiFi(){

     xTaskCreatePinnedToCore( 
         runWiFi,                                      // Task to handle special functions.
         "WiFiStart",                                  // name of task.
         1024*4,                                       // Stack size of task
         NULL,                                         // parameter of the task
         2,                                            // priority of the task
         &startwifiTask,                               // Task handle to keep track of created task 
         0 );                                          //core to run it on

  
}

//------------------------------------------------------------------------------
void netp2file(){

  size_t netpsize = netpass.size() * sizeof ( struct netp );

  memset( iv, 0, sizeof( iv ) );
  snprintf( (char *) iv, sizeof( iv) , RGiv); // RGkey and RGiv defined in WiFicredentials.h 
  memset( key, 0, sizeof( key ) );
  snprintf( (char *)key, sizeof( key), RGkey);

  uint8_t* cryptbuf = (uint8_t *) ps_calloc( netpsize , sizeof( uint8_t) );
 
  esp_aes_context ctx;
  esp_aes_init( &ctx );
  esp_aes_setkey( &ctx, key, 256 );

  uint8_t *plain = (uint8_t *)netpass.data();
  esp_aes_crypt_cbc( &ctx, ESP_AES_ENCRYPT, netpsize, iv, plain, cryptbuf );

/* //debug
  Serial.printf("Encrypted\n");
  for ( int i =0; i < netpsize; ++i ){
    Serial.printf( "%02x",cryptbuf[i]);
    if ( 0 == (i+1)%32)Serial.printf("\n"); 
  }
*/

  File file = multifs.open("/netpass", FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
  }else{
    Serial.printf("\nwritten %d bytes to file\n", file.write( cryptbuf, netpsize ) );    
  }
    
free( cryptbuf );
esp_aes_free( &ctx );

}

//------------------------------------------------------------------------------

void file2netp(){


memset( iv, 0, sizeof( iv ) );
snprintf( (char *) iv, sizeof( iv) ,"wlJIJ*d0kwdxm");
memset( key, 0, sizeof( key ) );
snprintf( (char *)key, sizeof( key), "woidjcowijc(*&(GUJHGV");

File file = multifs.open("/netpass");
if ( ! file ) {
    Serial.println( "No file found ");
    return;
}
size_t filesize = file.size();
uint8_t* cryptbuf = (uint8_t *) ps_calloc( filesize, sizeof( uint8_t) );

file.read( cryptbuf, filesize );

file.close();
/* //debug
Serial.printf("Encrypted - from file with %d bytes\n", filesize);
for ( int i =0; i < filesize ; ++i ){
    Serial.printf( "%02x",cryptbuf[i]);
    if ( 0 == (i+1)%32)Serial.printf("\n"); 
}
Serial.printf( "\n");
*/

esp_aes_context ctx;
esp_aes_init( &ctx );
esp_aes_setkey( &ctx, key, 256 );


uint8_t* plainbuf = (uint8_t *) ps_calloc( filesize, sizeof( uint8_t) );

esp_aes_crypt_cbc( &ctx, ESP_AES_DECRYPT, filesize, iv, cryptbuf, plainbuf );

struct  netp* n = (struct netp *) plainbuf;
size_t  plaincount = filesize / sizeof ( struct netp);

for ( int i = 0; i < plaincount; ++i ){
    netpass.push_back( *n );
    wifiMulti.addAP((char *)n->ssid, (char *)n->pass);
    ++n;
}

esp_aes_free( &ctx );
free( cryptbuf );
free( plainbuf );

}

/*
//----debug--------------------------------------------------------------------------

void displaynetp(){
  Serial.println("Display netpass");
  for ( auto n: netpass ){
    Serial.printf( "%s - %s\n", n.ssid, n.pass );
  }
}

//----debug-----------------------------------------------------------------------------
void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(fs.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}
*/
