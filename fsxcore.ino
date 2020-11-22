
#include "fsx.h"

 // SD Card | ESP32
 //    D2       12 // not for onebit
 //    D3       13 // not for onebit
 //    CMD      15
 //    VSS      GND
 //    VDD      3.3V
 //    CLK      14
 //    VSS      GND
 //    D0       2  (add 1K pull up after flashing)
 //    D1       43

std::vector<struct fsd> fsdlist;
//------------------------------------------------------------------------------
void refresh_fsd_usage(){
  for ( auto &efes : fsdlist ){   
            
    switch ( efes.fsnumber ){
    
      Serial.printf("fsnumber : %d\n", efes.fsnumber);  
      
 
      case FSDSD_MMC:
            efes.totalBytes = SD_MMC.totalBytes(); efes.freeBytes = SD_MMC.totalBytes() - SD_MMC.usedBytes(); efes.usedBytes = SD_MMC.usedBytes();
            break;
      case FSDSPIFFS:
            efes.totalBytes = SPIFFS.totalBytes(); efes.freeBytes = SPIFFS.totalBytes() - SPIFFS.usedBytes(); efes.usedBytes = SPIFFS.usedBytes();
            break;
      case FSDLITTLEFS:
            efes.totalBytes = LITTLEFS.totalBytes(); efes.freeBytes = LITTLEFS.totalBytes() - LITTLEFS.usedBytes(); efes.usedBytes = LITTLEFS.usedBytes();
            break;
            
      case FSDFFAT:
            efes.totalBytes = FFat.totalBytes(); efes.freeBytes = FFat.freeBytes(); efes.usedBytes =  FFat.totalBytes()- FFat.freeBytes();
            break;
      default:            
            break;
    }
  }
}



//------------------------------------------------------------------------------
void mount_fs(){
 
  String output;
  
 
  if( SD_MMC.begin("/sdcard", true) ){ // onebit mode change to fals for 2bitmode
    fsdlist.push_back( (struct fsd){ &SD_MMC,"SD_MMC","/sdcard",FSDSD_MMC,SD_MMC.totalBytes(),SD_MMC.totalBytes()-SD_MMC.usedBytes(), SD_MMC.usedBytes()} );        
    Serial.printf( "sd mmc totalbytes: %llu, used bytes %llu\n", SD_MMC.totalBytes(), SD_MMC.usedBytes() );
 //   delay(1000);
  }
  
  if( SPIFFS.begin() ){
    fsdlist.push_back( (struct fsd){ &SPIFFS,"SPIFFS","/spiffs",FSDSPIFFS,SPIFFS.totalBytes(),SPIFFS.totalBytes()-SPIFFS.usedBytes(), SPIFFS.usedBytes()} );        
//    delay(1000);
  }
  if( LITTLEFS.begin() ){
    fsdlist.push_back( (struct fsd){ &LITTLEFS, "LITTLEFS","/littlefs",FSDLITTLEFS, LITTLEFS.totalBytes(), LITTLEFS.totalBytes()-LITTLEFS.usedBytes(), LITTLEFS.usedBytes()} );        
//    delay(1000);
 }
 
  if( FFat.begin() ){   
    fsdlist.push_back( (struct fsd){ &FFat,"FFat","/ffat",FSDFFAT,FFat.totalBytes(),FFat.freeBytes(), FFat.totalBytes()- FFat.freeBytes()}  );        
//    delay(1000);
  }

  if ( psramFound() ){
    lister.open();
    directory.open("/directory.html");
    uploadform.open("/upload.html");
    settingsform.open("/settings.html");
    logo.open("/favicon.ico");
  }  
}
//------------------------------------------------------------------------------
char  *nameoffs( fs::FS *ff ){

  for ( auto &efes : fsdlist ){           
      if (  efes.f == ff )return efes.fsname;
  } 

return( NULL );
}   
//------------------------------------------------------------------------------
void maketargetpath( String& filename, String& targetdir, String& newpath ){

     newpath  = targetdir + "/";
     newpath += filename.substring( filename.lastIndexOf( '/' ) + 1 );
}
//------------------------------------------------------------------------------
fs::FS  *fsfromfile( const char *fullfilename ){

  for ( auto &efes : fsdlist ){           
      if (  strncmp( efes.fsmount, fullfilename,strlen( efes.fsmount ) ) == 0 )return efes.f;
  } 

return( NULL );
}   

//---------------------------------------------------------------------------

bool isdir( const char  *fullfilename ){
    DIR *dir = opendir(fullfilename);
    if (!dir){
        //Serial.printf( "%s is not a directory. Cannot list.\n", basepath);
        return(false);
    }
    closedir( dir );
    return(true);
}
//---------------------------------------------------------------------------
bool sourceintarget( String& sourcepath, String& targetpath ){
  if ( !isdir( sourcepath.c_str() )  )return(false);  

  if ( targetpath.startsWith( sourcepath + "/" ) ) return(true);
  return(false);
}

//---------------------------------------------------------------------------
void deltree( fs::FS *ff, const char *path ){

    fs::File dir = ff->open( path );

    if ( !dir )return;
    
    if(!dir.isDirectory()){
       Serial.printf("%s is a file\n", path);
       dir.close();
       Serial.printf( "result of removing file %s: %d\n", path, ff->remove( path ) );
       return;
    }
    
    Serial.printf("%s is a directory\n", path);
     
    fs::File entry, nextentry;
       
    while ( entry = dir.openNextFile() ){          
      
      if ( entry.isDirectory()  ){
         deltree( ff, entry.name() );  
         esp_task_wdt_reset();      
      } else{ 
         char* tmpname = strdup( entry.name() );
         entry.close();// openNextFile opens the file, LittleFS will refuse to delete the file if open;
                       // FAT doesn't mind
         Serial.printf( "result of removing file %s: %d\n", tmpname, ff->remove( tmpname ) );
         free( tmpname );
         esp_task_wdt_reset();
      }     
      
    }

    dir.close(); 
    Serial.printf( "result of removing directory %s: %d\n", path, ff->rmdir( path ) );     
}

//------------------------------------------------------------------------------
// returns 0 - ok
// returns 1 - source directory not found
// returns 2 - source file not found by copyfile
// return  3 - source directory in target directory  
// return  4 - no memory for read buffer  
// return  5 - error reading source file  
// return  6 - error writing target file 
// return  7 - error removing target directory


int copytree( fs::FS *sourcefs, String &sourcefspath, fs::FS *targetfs, String &targetfspath){
    int result;

    if ( sourcefspath.length() == 0 ) sourcefspath = "/";
    Serial.println("Copying tree from [" + sourcefspath + "]");
    fs::File dir = sourcefs->open( sourcefspath );
    if ( !dir ){
      Serial.println("failed to open inputdirectory" + sourcefspath );
      return(1);
    }

    // copyfile will copy the file if source is a file, or create a directory if source is a directory 
    result = copyfile( sourcefs, sourcefspath, targetfs, targetfspath);
    esp_task_wdt_reset();
    
    if(!dir.isDirectory() || result ){
       if( !dir.isDirectory() ) Serial.println(sourcefspath + " is a file\n");
       dir.close();
       return( result);
    }
    
    Serial.println( sourcefspath + " is a directory\n");
     
    fs::File entry;
    while ( entry = dir.openNextFile() ){          

      String tmpname = String(entry.name());
      entry.close();// openNextFile opens the file, LittleFS will refuse to delete the file if open;
                       // FAT doesn't mind
      result = copytree( sourcefs, tmpname, targetfs, targetfspath + "/" + tmpname.substring( tmpname.lastIndexOf( '/' ) + 1 ) );                 
      if ( result ) return( result );    
      
    }

    dir.close(); 
    return(0);
}


//------------------------------------------------------------------------------
int copyfile( fs::FS *sourcefs, String &sourcefspath, fs::FS *targetfs, String &targetfspath){

  size_t  filesize, bytesread=0, totalbytesread=0,totalbyteswritten=0, byteswritten=0, writeresult;
  size_t  readbuffer_size = (1024*32);
  uint8_t *readbuffer; 

  Serial.println( "Copy "+ sourcefspath + " to "+ targetfspath );
        
  File sourcefile = sourcefs->open ( sourcefspath, FILE_READ ); 
  if ( !sourcefile ){ Serial.println( "Error opening "+ sourcefspath + "for read "); return( 2 );} 

  if ( sourcefile.isDirectory() ){
        sourcefile.close();
        if ( sourcefs == targetfs && sourceintarget( sourcefspath, targetfspath ) ){
           return( 3);
        }
        
        if ( !targetfs->exists( targetfspath ) ){
          if ( ! targetfs->mkdir( targetfspath ) )return( 7 );        
        }
  }

  if ( psramFound() ){
    Serial.printf( "Allocating %u bytes of psram\n", readbuffer_size);
    readbuffer = (uint8_t *)ps_malloc( readbuffer_size );
  }else{
    readbuffer_size = 1024;
    Serial.printf( "Allocating %u bytes from heap\n", readbuffer_size);
    readbuffer = (uint8_t *)malloc( readbuffer_size );    
  }
  
  if ( ! readbuffer ){
     sourcefile.close();
     Serial.println( "Error allocating memory "); 
     return( 4 );
  }
  
  File targetfile = targetfs->open ( targetfspath, FILE_WRITE ); 
  if ( !targetfile ){ sourcefile.close(); Serial.println( "Error opening "+ targetfspath + "for write"); return( false);} 

  filesize = sourcefile.size();

  while( totalbyteswritten < filesize ){
   
      bytesread = sourcefile.read( readbuffer, (filesize - totalbytesread)> readbuffer_size ? readbuffer_size :(filesize - totalbytesread) );
      if ( bytesread < 0 ){ 
         sourcefile.close(); 
         targetfile.close(); 
         free(readbuffer);
         targetfs->remove( targetfspath );
         Serial.println( "Error reading  "+ sourcefspath); 
         return(5);
      }

      totalbytesread += bytesread;
      esp_task_wdt_reset();
      
      byteswritten = 0;
      while( byteswritten < bytesread ){
             writeresult= targetfile.write( &readbuffer[ byteswritten ], bytesread - byteswritten );    
             if ( writeresult == 0 ){
                 sourcefile.close(); 
                 targetfile.close();
                 free(readbuffer); 
                 targetfs->remove( targetfspath );
                 Serial.println( "Error writing  "+ targetfspath); 
                 return(6);
             }
             byteswritten += writeresult;
      }
      totalbyteswritten += byteswritten;      
  }

  sourcefile.close(); 
  targetfile.close(); 
  free(readbuffer);
  
  return(0);
}
//------------------------------------------------------------------------------
size_t fileoffset( const char *fullfilename ){   
    const char *s;  
    for ( s= fullfilename + 1; *s != '/';++s );
    return( s - fullfilename ); 
}


//------------------------------------------------------------------------------

int makepath( fs::FS &ff, String &pstring ){
    char *s, *pathstring = (char *)pstring.c_str();
    int  mode=0, slashcount=0;
    
    s = pathstring;
    
    while ( *s ){
        switch( mode ){
             case 0: // if file system included switch mode at slashcount 2 instead of 1   
                    if ( *s == '/')++slashcount;
                    if ( slashcount == 1 ){ mode = 1; }
                    break;
             case 1:
                    if ( *s == '/'){
                        *s = 0;
                        
                        if ( !ff.mkdir( pathstring ) ){
                          Serial.printf( "Failed to create directory %s\n", pathstring);
                          return(0);
                        }
                        Serial.printf( "Created directory %s\n", pathstring);
                        *s = '/';
                    }
                    break;
        }    
        ++s;
    }
    
    Serial.printf( "Directories created for %s\n", pathstring);
    return(1);
}        
