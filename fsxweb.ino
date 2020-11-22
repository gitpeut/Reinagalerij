#include "fsx.h"

AsyncWebServer  fsxserver(80);
TaskHandle_t    FSXServerTask;
SemaphoreHandle_t updateSemaphore;

//------------------------------------------------------------------------------------
// added esp_task_wdt_reset(); in some tight loops against task watchdog panics.

void getFS(){
    xSemaphoreTake( updateSemaphore, portMAX_DELAY);
}

void loseFS(){
    xSemaphoreGive( updateSemaphore);
}


//==============================================================
//                     URL Encode Decode Functions
//https://circuits4you.com/2019/03/21/esp8266-url-encode-decode-example/
//==============================================================
String urldecode(String str)
{
    
    String encodedString="";
    char c;
    char code0;
    char code1;
    for (int i =0; i < str.length(); i++){
        c=str.charAt(i);
      if (c == '+'){
        encodedString+=' ';  
      }else if (c == '%') {
        i++;
        code0=str.charAt(i);
        i++;
        code1=str.charAt(i);
        c = (h2int(code0) << 4) | h2int(code1);
        encodedString+=c;
      } else{
        
        encodedString+=c;  
      }
      
      yield();
    }
    
   return encodedString;
}
 
String urlencode(String str)
{
    String encodedString="";
    char c;
    char code0;
    char code1;
    char code2;
    for (int i =0; i < str.length(); i++){
      c=str.charAt(i);
      if (c == ' '){
        encodedString+= '+';
      } else if (isalnum(c)){
        encodedString+=c;
      } else{
        code1=(c & 0xf)+'0';
        if ((c & 0xf) >9){
            code1=(c & 0xf) - 10 + 'A';
        }
        c=(c>>4)&0xf;
        code0=c+'0';
        if (c > 9){
            code0=c - 10 + 'A';
        }
        code2='\0';
        encodedString+='%';
        encodedString+=code0;
        encodedString+=code1;
        //encodedString+=code2;
      }
      yield();
    }
    return encodedString;
    
}
//------------------------------------------------------------------------------------
 
unsigned char h2int(char c)
{
    if (c >= '0' && c <='9'){
        return((unsigned char)c - '0');
    }
    if (c >= 'a' && c <='f'){
        return((unsigned char)c - 'a' + 10);
    }
    if (c >= 'A' && c <='F'){
        return((unsigned char)c - 'A' + 10);
    }
    return(0);
}
//------------------------------------------------------------------------------------
//format bytes
String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}
//---------------------------------------------------------------------------

String getContentType(String filename) {
  
  if (filename.endsWith(".htm")) {
    return "text/html";
  } else if (filename.endsWith(".html")) {
    return "text/html";
  } else if (filename.endsWith(".css")) {
    return "text/css";
  } else if (filename.endsWith(".js")) {
    return "application/javascript";
  } else if (filename.endsWith(".png")) {
    return "image/png";
  } else if (filename.endsWith(".gif")) {
    return "image/gif";
   } else if (filename.endsWith(".bmp")) {
    return "image/bmp";
  } else if (filename.endsWith(".jpg")) {
    return "image/jpeg";
  } else if (filename.endsWith(".ico")) {
    return "image/x-icon";
  } else if (filename.endsWith(".xml")) {
    return "text/xml";
  } else if (filename.endsWith(".pdf")) {
    return "application/x-pdf";
  } else if (filename.endsWith(".zip")) {
    return "application/x-zip";
  } else if (filename.endsWith(".gz")) {
    return "application/x-gzip";
  }else if (filename.endsWith(".wav")) {
    return "audio/wav";
  }else if (filename.endsWith(".mp3")) {
    return "audio/mp3";
  }else if (filename.endsWith(".m4a")) {
    return "audio/m4a";
  }else if (filename.endsWith(".flac")) {
    return "audio/flac";
  }
  return "text/plain";
}


//--------------------------------------------------------------------------------

void handleFileRead(  AsyncWebServerRequest *request ) {

  String path = request->url();
  
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) {
    path += "index.htm";
  }
  path = urldecode( path);
  
  fs::FS  *ff = fsfromfile( path.c_str() );

  String fspath;
  
  if ( ff == NULL ){
      ff = fsdlist.back().f;
      //flash filesystems added last to fsdlist, due ffat bug
      Serial.printf("Fallback to filesystem %s\n", fsdlist.back().fsname );     
      fspath = path;
  }else{
      fspath = path.substring( fileoffset( path.c_str()) );
  }


    
  String contentType = getContentType(path);
  String pathWithGz = fspath + ".gz";
  
  getFS();
  
  if ( ff->exists(pathWithGz) || ff->exists(fspath)) {
    if ( ff->exists(pathWithGz)) {
      fspath += ".gz";
    }

    if ( request->hasParam("download") )contentType = "application/octet-stream";

    request->send( *ff, fspath, contentType );

    loseFS();
    Serial.printf("File has been streamed\n");
    
    return;
  }
  loseFS();
  request->send( 404, "text/plain", "FileNotFound"); 
}

//--------------------------------------------------------------------------------
void handleRename( AsyncWebServerRequest *request ) {

  String  oldname,newname;
  String  sourcefspath, targetfspath;
  fs::FS  *sourcefs=NULL, *targetfs=NULL;


  lister.close();
  
  if ( request->hasParam("oldname")  && request->hasParam("newname") ) {
      
      oldname    = request->getParam("oldname")->value();
      newname    = request->getParam("newname")->value();

      if ( oldname == newname){
        request->send(400, "text/plain", "old and new filename are the same, no rename necessary" );
        return;
      }

      getFS();
      
      sourcefspath  = oldname.substring( fileoffset( oldname.c_str()) );
      targetfspath  = newname.substring( fileoffset( newname.c_str()) );

      sourcefs   = fsfromfile( oldname.c_str() ); 
      targetfs   = fsfromfile( newname.c_str() ); 

      if ( targetfs == NULL || sourcefs == NULL || sourcefs != targetfs){
        loseFS();
        request->send(500, "text/plain", "Filesystem name invalid or not the same" );
        return;
      }

      if ( !targetfs->rename( sourcefspath, targetfspath ) ){
        loseFS();
        request->send(500, "text/plain", "Rename of " + oldname + " to " + newname + " failed ");
        return;
      }
  }else{
      request->send(500, "text/plain", "Argument oldname and newname are missing");
      return;
  }  

loseFS();

request->send(200, "text/plain", "Renamed " + oldname +" to " + newname);
}

//--------------------------------------------------------------------------------
void handleMultiDelete( AsyncWebServerRequest *request ) {

  String  varname;
  int     i,filecount;
  String  path, fspath;
  fs::FS  *ff;

  getFS(); 
  lister.close();
  
  if ( request->hasParam("filecount", true) ){
      filecount = atoi( request->getParam("filecount",true)->value().c_str() ) ;
      Serial.printf("Deleting %d files\n", filecount);
  }else{
      loseFS();
      request->send(500, "text/plain", "Argument filecount is missing");
      return;
  }  

  for( i=0; i< filecount; ++i ){
     varname = "file" + String(i); 
     path = request->getParam( varname,true)->value();

     ff = fsfromfile( path.c_str() );
     if ( ff == NULL ){
      Serial.println("No filesystem found in " + varname + "=" + path);
      loseFS();
      request->send(400, "text/plain", "No mounted filesystem in path, full filename required starting with mounted filesystem, either /sd,/littlefs,/spiffs or /ffat");
      return;
     } 

     fspath = path.substring( fileoffset( path.c_str()) );

     if (fspath == "/") {
      loseFS();
      request->send(500, "text/plain", "/ is not a valid filename");
      return;
    }
    
    if (!ff->exists(fspath)) {
      loseFS();
      request->send(404, "text/plain", path + " Not Found " );
      return;
    }

    deltree( ff, fspath.c_str() );       
  }

loseFS();
request->send(200, "text/plain", "All files succesfully deleted");

}

//--------------------------------------------------------------------------------
void handleFileDelete(AsyncWebServerRequest *request) {

  if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for delete");
    return;
  }

  AsyncWebParameter* p = request->getParam(0);
  String path = p->value();

  getFS();
  lister.close();
  
  path = urldecode( path);
  Serial.println("handleFileDelete: " + path);
  
  fs::FS  *ff = fsfromfile( path.c_str() );
  int     returncode = 200;
  String  fspath;
  
  if ( ff == NULL ){
      Serial.printf("No filesystem found\n");
      loseFS();  
      request->send(400, "text/plain", "No valid filesystem in path, full filename required starting with /sd,/littlefs,/spiffs or /ffat");
      return;
  }

  fspath = path.substring( fileoffset( path.c_str()) );

    
  if (fspath == "/") {
      loseFS();
      request->send(500, "text/plain", "/ is not a valid filename");
      return;
  }
  if (!ff->exists(fspath)) {
    loseFS();
    request->send(404, "text/plain", "FileNotFound");
    return;
  }

  deltree( ff, fspath.c_str() ); 

  loseFS();
  request->send(200, "text/plain", path  + " succesfully deleted");
  
}

//-------------------------------------------------------------------------

void handleMkdir(AsyncWebServerRequest *request){

  if ( request->params() == 0) {
    request->send(500, "text/plain", "BAD ARGS for mkdir");
    return;
  }

  getFS();
  lister.close();
 
  String path = request->getParam("subdir")->value();

  path = urldecode( path);
  Serial.println("handleMkdir: " + path);
  
  fs::FS  *ff = fsfromfile( path.c_str() );
  String  fspath;

  if ( ff == NULL ){
      loseFS();
      Serial.printf("No filesystem found\n");
      request->send(400, "text/plain", "No valid mounted filesystem in path, full filename required starting with /sd,/littlefs,/spiffs or /ffat");
      return;
  }

  fspath = path.substring( fileoffset( path.c_str()) );
  if (fspath == "/") {
    loseFS();
    request->send(400, "text/plain", "/ is not a valid filename");
    return;
  }

  makepath ( *ff, fspath );  
  if ( ! ff->mkdir( fspath ) ){
    loseFS();
    request->send(400, "text/plain", "failed to create subdirectory " + path );       
    return;
  }

  loseFS();
  request->send(200, "text/plain", "Directory " + path + " succesfully created");

}
//------------------------------------------------------------------------------------
void handleMove(AsyncWebServerRequest *request) {

  String  varname;
  int     i,filecount;
  String  targetdir, sourcepath, targetpath;
  String  sourcefspath, targetfspath;
  bool    removesource, docopy;
  fs::FS  *sourcefs=NULL, *targetfs=NULL;

  getFS();
  lister.close();
  
  if ( request->hasParam("filecount", true) && request->hasParam("targetdirectory",true) && request->hasParam("removesource",true) ){
      
      filecount    = atoi( request->getParam("filecount",true)->value().c_str() ) ;
      targetdir    = request->getParam("targetdirectory",true)->value();
      targetfs     = fsfromfile( targetdir.c_str() );
      removesource = (bool) atoi(request->getParam("removesource",true)->value().c_str());
      if ( targetfs == NULL ) {
        loseFS();       
        request->send(500, "text/plain", "No such target directory, filesystem not found" );
        return;
      }
      
  
      Serial.printf( "%s %d file%s to %s\n", removesource?"Moving":"Copying", filecount,(filecount == 1)?"":"s", targetdir.c_str());
  }else{
      loseFS();       
      return request->send(500, "text/plain", "Argument filecount, targetdiretory and removesource are missing");
  }  

  for( i=0; i< filecount; ++i ){
     varname = "file" + String(i); 
     
     sourcepath = request->getParam( varname,true)->value();
     
     sourcefs = fsfromfile( sourcepath.c_str() );
      
     if ( sourcefs == NULL ){
      Serial.println("No filesystem found in " + varname + "=" + sourcepath);
        loseFS();
        request->send(400, "text/plain", "No mounted filesystem in path, full filename required starting with mounted filesystem, either /sd,/littlefs,/spiffs or /ffat");
      return;
     } 

     maketargetpath( sourcepath, targetdir, targetpath );
     targetfs = fsfromfile( targetpath.c_str() );

     docopy = false; 
     if ( sourcefs != targetfs || !removesource ) docopy = true;
     
     sourcefspath  = sourcepath.substring( fileoffset( sourcepath.c_str()) );
     targetfspath  = targetpath.substring( fileoffset( targetpath.c_str()) );

     if ( sourceintarget( sourcepath, targetpath ) ){  
        loseFS();
        request->send(400, "text/plain", "Cannot pretzel source in target path");      
        return;
     }
      
     if ( docopy ){
        int result;
        if ( result = copytree( sourcefs, sourcefspath, targetfs, targetfspath) ){
          loseFS();
          request->send(500, "text/plain", "Copy of " + sourcepath + " to " + targetpath + " failed " + FPSTR( cperror[ result ] ) );
          return;
        }
     }else{
        makepath ( *targetfs, targetfspath );
        if ( !targetfs->rename( sourcefspath, targetfspath ) ){
          loseFS();
          request->send(500, "text/plain", "Move of " + sourcepath + " to " + targetpath + " failed ");
          return;
        }
        esp_task_wdt_reset();
     }

     if ( docopy && removesource ){
        deltree( sourcefs, sourcefspath.c_str() );
     }
           
  }
loseFS();
request->send(200, "text/plain", "All files succesfully moved");

}


//----------------------------------------------------------------------------
void send_logo( AsyncWebServerRequest *request ){
  if(!logo.open("/favicon.ico") ){
    request->send(503, "text/plain", "Error while listing logo" );
  }else{
    Serial.printf("Streaming logo, size = %d\n", logo.size());
    request->send( logo, "image/x-icon", logo.size() );
  }  
}
//----------------------------------------------------------------------------
void showUploadform(AsyncWebServerRequest *request) {

  if( ! uploadform.open() ){
    request->send(503, "text/plain", "Error while listing uploadform" );
  }else{
    request->send( uploadform, "text/html", uploadform.size() );
  }
}

//---------------------------------------------------------------------------

void handleFileUpload(AsyncWebServerRequest * request, String filename, size_t index, uint8_t *data, size_t len, bool final){
 
  static File fsUploadFile;
  String      fspath;
  static int  bcount, bmultiplier;
  
  if(!index){

        getFS();
                
        Serial.printf("UploadStart: %s\n", filename.c_str());
        lister.close();

        String path = filename;
        if (!path.startsWith("/")) {
          path = "/" + path;
        }
        
        Serial.print("handleFileUpload Name: "); 
        Serial.println(path);
    
        fs::FS  *ff = fsfromfile( path.c_str() );
        
        if ( ff == NULL ){
          ff = fsdlist.back().f;
          //flash filesystems added last to fsdlist, due ffat bug
          path = fsdlist.back().fsmount + path;
          Serial.printf("Upload fallen back to filesystem %s\n", fsdlist.back().fsname ); 
        }
       
        fspath = path.substring( fileoffset( path.c_str()) );
        
        if (fspath == "/") {
          loseFS();
          request->send(500, "text/plain", "/ is not a valid filename");
          return;
        }
         
        Serial.printf( "Making all directories for %s on %s\n",  fspath.c_str(), nameoffs( ff ) );
        Serial.println( "Length of filename " + fspath + " " + String( fspath.length()) );
            
        makepath ( *ff, fspath );
        bcount=0; bmultiplier = 1;
        
        fsUploadFile = ff->open(fspath, "w");
        if ( !fsUploadFile  ){
              loseFS();
              Serial.println("Error opening file " + fspath + " " + String (strerror(errno)) );
              request->send(400, "text/plain", String("Error opening ") + fspath + String(" ") + String ( strerror(errno) ) );
              return;
        }
  }
  int olderrno = errno;
  if (fsUploadFile) {
      if ( fsUploadFile.write( data, len) < len ){
           fsUploadFile.close();
           if ( olderrno != errno ){
            loseFS();
            request->send(500, "text/plain", "Error during write : " + String ( strerror(errno) ) );
            Serial.println("Error writing file " +fspath + " " + String (strerror(errno)) );
            return;
           }
      }
      esp_task_wdt_reset();
      bcount +=len;
      while ( bcount > (bmultiplier * 4096) ){ 
        Serial.print("*");
        if ( (bmultiplier%60) == 0) Serial.println("");
        bmultiplier++;
      }
      
  }

  if ( final ){
    if (fsUploadFile) {
      fsUploadFile.close();        
    }
    loseFS();

    Serial.printf("Uploaded file of %u bytes\n", bcount);
  }
  
}

//---------------------------------------------------------------------
void send_json_status(AsyncWebServerRequest *request)
{
 
char uptime[32];
int sec = millis() / 1000;
int upsec,upminute,uphr,updays;
 
upminute = (sec / 60) % 60;
uphr     = (sec / (60*60)) % 24;
updays   = sec  / (24*60*60);
upsec    = sec % 60;

sprintf( uptime, "%d %02d:%02d:%02d", updays, uphr, upminute,upsec);

  String output = "{\r\n";

  output += "\t\"Hostname\" : \"";
  output += esphostname;
  output += "\",\r\n";

  output += "\t\"Compiletime\" : \"";
  output += compile_time;
  output += "\",\r\n";

  String srcfile = String( sourcefile );
  srcfile.replace("\\", "\\\\");
  output += "\t\"Sourcefile\" : \"";
  output += srcfile;
  output += "\",\r\n";

  output += "\t\"uptime\" : \"";
  output += uptime;
  output += "\",\r\n";
    
  output += "\t\"uptime\" : \"";
  output += uptime;
  output += "\",\r\n";

  output += "\t\"RAMsize\" : ";
  output += ESP.getHeapSize();
  output += ",\r\n";
  
  output += "\t\"RAMfree\" : ";
  output += ESP.getFreeHeap();
  output += ",\r\n";

  output += "\t\"Settings\" : ";

  char* stmp = (char *) ps_calloc( strlen( settings_format ) + 32, 1 );
  sprintf( stmp,settings_format, 
          settings.portland, 
          settings.pauseseconds, 
          settings.adapt_rotation, 
          settings.portrait, 
          settings.landscape);

  output += stmp;
  output += "\r\n";
   
  output += "}" ;
    
  request->send(200, "application/json;charset=UTF-8", output);
  free( stmp);
}

//----------------------------------------------------------------------------
void handleRoot(AsyncWebServerRequest *request) {

  if( ! directory.open() ){
    request->send(502, "txt/plain", "Error while sending directory.html" );
  }else{
    request->send( directory, "text/html", directory.size() );
  }
}

//----------------------------------------------------------------------------
void handleFileList( AsyncWebServerRequest *request ) {

  
  if( ! lister.open() ) {
    request->send(500, "txt/plain", "Error while listing" );
  }else{
    request->send( lister, "text/html", lister.size() );
  }
}

//---------------------------------------------------------------------------------------

void send_settings( AsyncWebServerRequest *request ) {
  
  if( ! settingsform.open() ){
    request->send(503, "text/plain", "Error while opening settingsform" );
  }else{
    request->send( settingsform, "text/html", settingsform.size() );
  }
}
//----------------------------------------------------------------------------
void handleSettings(AsyncWebServerRequest *request){
   struct appconfig z = settings;
   AsyncWebParameter* p;
   
   if ( request->hasParam("portland", true) ){
       int pl = atoi( request->getParam("portland",true)->value().c_str() ) ;
       if ( pl != 0 && pl != 1 ){ request->send(400, "txt/plain", "portland can only be 0 or 1" ); return;}
       z.portland = pl;
   }

   if ( request->hasParam("pauseseconds",true) ){
      z.pauseseconds = atoi( request->getParam("adapt_rotation",true)->value().c_str() ) ;      //p = request->getParam("pauseseconds" );
      if ( z.pauseseconds < 0 ) {request->send(400, "txt/plain", "pauseseconds must be greater than 0" ); return;}
   }

   if ( request->hasParam("adapt_rotation",true) ){
      int ar = atoi( request->getParam("adapt_rotation",true)->value().c_str() ) ;
      if ( ar != 0 && ar != 1 ) {request->send(400, "txt/plain", "adapt_rotation can only be 0 or 1" );return;}
      z.adapt_rotation = ar;
   }

   if ( request->hasParam("portrait",true) ){
      z.portrait = atoi( request->getParam("portrait",true)->value().c_str() ) ;
      if ( z.portrait != 0 && z.portrait != 2 ) {request->send(400, "txt/plain", "portrait can only be 0 or 2" );return;}
   }

   if ( request->hasParam("landscape",true) ){
      z.landscape = atoi( request->getParam("landscape",true)->value().c_str() ) ;
      if ( z.landscape != 1 && z.landscape != 3 ) {request->send(400, "txt/plain", "landscape can only be 1 or 3" );return;}
   }
   
request->send(200, "txt/plain", "ok" );
settings = z;
getFS();
write_config();
loseFS();
Serial.println("Written new settings");
}
//--------------------------------------------------------------------------

void handleUpdate(AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
  
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  
  if (!index){
  
    Serial.println("Update");

    getFS();
    //Update.runAsync(true);
    //content_len = request->contentLength();
    // if filename includes spiffs, update the spiffs partition
    //int cmd = (filename.indexOf("spiffs") > -1) ? U_PART : U_FLASH;
    if (!Update.begin( free_space )) {
      Update.printError(Serial);
    }
  }

  if (Update.write(data, len) != len) {
    Update.printError(Serial);
  }
  esp_task_wdt_reset();

  if (final) {
    loseFS();
    AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "Please wait while the device reboots");
    response->addHeader("Refresh", "20");  
    response->addHeader("Location", "/");
    request->send(response);
    if (!Update.end(true)){
      Update.printError(Serial);
    } else {
      Serial.println("\nUpdate complete");
      Serial.flush();
      delay(100);
      ESP.restart();
    }
  }
}
//---------------------------------------------------------------------------------------
void printProgress(size_t progress, size_t size) {
  size_t percent = (progress*1000)/size;
  if ( (percent%100)  == 0 )Serial.printf(" %u%%\r", percent/10);
}

//---------------------------------------------------------------------------------------
void FSXServer(void *param){

  fsxserver.on("/", handleRoot);
  
  fsxserver.on("/list", HTTP_GET, handleFileList);
  fsxserver.on("/move", HTTP_POST, handleMove); 
  fsxserver.on("/rename", HTTP_GET, handleRename);
  
  fsxserver.on("/mkdir", HTTP_GET, handleMkdir);
  fsxserver.on("/delete", HTTP_GET, handleFileDelete);
  fsxserver.on("/delete", HTTP_POST, handleMultiDelete);
  
  fsxserver.on("/upload", HTTP_GET, showUploadform);
  fsxserver.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request){ request->send(200, "text/html", uploadpage); }, handleFileUpload);

  fsxserver.on("/settings", HTTP_GET, send_settings);
  fsxserver.on("/settings", HTTP_POST, handleSettings);
  
  fsxserver.on("/status", HTTP_GET, send_json_status );
  fsxserver.on("/favicon.ico", send_logo );

  fsxserver.onNotFound( handleFileRead );

  fsxserver.on("/reset", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send(200, "text/plain", "Doej!");
        delay(20);
        ESP.restart();
  });

  fsxserver.on("/update", HTTP_GET, []( AsyncWebServerRequest *request ) {
        request->send( 200, "text/html", updateform );
  });  
  fsxserver.on("/update", HTTP_POST, [](AsyncWebServerRequest *request){ request->send( 200, "text/html",updateform); }, handleUpdate);
 
  fsxserver.begin();
  Serial.printf( "sd mmc totalbytes: %llu, used bytes %llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes() );
  Update.onProgress(printProgress);
  
  while(1){
 //   fsxserver.handleClient();
    delay(100);
  }
  
}

//----------------------------------------------------------------------------
void stopFSXServer(){
  
  fsxserver.end();
  vTaskDelete( FSXServerTask ); 

  return;
}
//----------------------------------------------------------------------------

void startFSXServer(){

     xTaskCreatePinnedToCore( 
         FSXServer,                                      // Task to handle special functions.
         "FSX Webserver",                                  // name of task.
         1024*8,                                       // Stack size of task
         NULL,                                         // parameter of the task
         4,                                            // priority of the task
         &FSXServerTask,                               // Task handle to keep track of created task 
         0 );                                          //core to run it on
 
}
