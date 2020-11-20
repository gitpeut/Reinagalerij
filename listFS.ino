#include "fsx.h"

listFS  lister;
listFS  directory;
listFS  uploadform;
listFS  settingsform;
listFS  logo;

listFS::listFS(){
    psbuffer = NULL;
    endpsbuffer = NULL;
    readpointer = NULL;
    psbuffer_length = -1;
    abort_listing=0; 
    listname[0] = 0; 
}

listFS::~listFS(){
 close();
}

//-----------------------------------------------------

bool listFS::open( String oname, const char *mode)
{
  
 if ( mode != FILE_READ ) return( false );  
 if ( psbuffer_length != -1 ){  
  readpointer = psbuffer;
  return(true);
 }

  fs::FS *sourcefs;
  File sourcefile;
  size_t filesize;
  
  if( oname[0] ) {
    Serial.printf("listFS-Starting file buffer for %s\n", oname.c_str());

    strcpy( listname, oname.c_str() );
   
    sourcefs    = fsfromfile( listname );
    if ( sourcefs == NULL ) sourcefs = fsdlist.back().f;
     
    sourcefile  = sourcefs->open ( listname, FILE_READ ); 
    if ( !sourcefile ){ Serial.printf( "Error opening %s for read ", listname); return( false );} 
    
    filesize    = sourcefile.size();
    Serial.printf( "Opened %s for read, filesize %u \n",listname, filesize);
    
    psbuffer = (uint8_t *) ps_calloc( filesize ,1 );

  }else{
    Serial.println("listFS-Starting lister");

    strcpy( listname, "lister" );
    psbuffer = (uint8_t *) ps_calloc( 1000000,1 );
  }
  
  if( !psbuffer){
    Serial.println("Could not calloc psram\n");
    if ( oname[0] ) sourcefile.close(); 
    return(false );
  }

  endpsbuffer = psbuffer;
  readpointer = psbuffer;
  
  
  if( oname[0] == 0) {
    Serial.println("Starting all2fsjson");
    allfs2json( (char **) &endpsbuffer );
    psbuffer_length = endpsbuffer-psbuffer;
 
  }else{
      Serial.println("Reading file");
        
      size_t  bytesread = 0, totalbytesread=0;
   
      while( sourcefile.available() ){   
        bytesread = sourcefile.read( endpsbuffer, (filesize - totalbytesread) );
        if ( bytesread < 0 ){ 
             sourcefile.close(); 
             close();
             Serial.printf( "Read returned < 0 while reading file %s ",listname ); 
             return( false);
          }
          
          totalbytesread += bytesread;
          endpsbuffer += bytesread;
      }
      sourcefile.close();
      psbuffer_length = endpsbuffer-psbuffer;

  }
  
  Serial.printf("Buffered %u bytes\n", psbuffer_length);

  return( true );
}

//-----------------------------------------------------

void listFS::close(){
  if ( psbuffer ){
    free( psbuffer );
    psbuffer = NULL;
    endpsbuffer = NULL;
    readpointer = NULL;
    psbuffer_length = -1;
  }    
}  

//------------------------------------------------------------------------------

size_t listFS::readBytes( uint8_t *buffer, size_t length = 1 ){
  size_t copylength;

  if( readpointer == NULL ) return 0;
  
  if( readpointer + length < endpsbuffer ){
    copylength = length;
  }else{
    copylength = endpsbuffer - readpointer;
  }
  
  for( int i=0; i < copylength ; ++i ){
      *(buffer + i) = *readpointer++;
  }
  
  return copylength;
}   

int listFS::read(){ 
  if ( readpointer < endpsbuffer ){
    return *readpointer++; 
  }
  return( -1 );  
}
int listFS::peek(){
  if ( readpointer < endpsbuffer ){
    return *readpointer; 
  } 
  return( -1 );
}

//------------------------------------------------------------------------------

void listFS::addtos ( char **s, const char *added){
    strcpy( *s, added); //only works with readable text like json data
    *s += strlen(added);            
}

//------------------------------------------------------------------------------

void listFS::fs2json( fs::FS &fs, const char *fsmount, const char *dirname, char **s , int depth){

fs::File dir = fs.open( dirname );
String output;

if(!dir.isDirectory()){
    Serial.println(" - not a directory");
    return;
}

String tabs = "";
for ( int i=0 ; i < depth; ++i ) tabs += "   ";
    
output = "\n" + tabs +"[";addtos ( s, output.c_str() );
      
bool first = true;

fs::File entry;

while ( (entry = dir.openNextFile()) ){
         if ( abort_listing ) return;
         if ( first ){
            first = false;
         }else{
            output = ",\n" + tabs;addtos ( s, output.c_str() );
      
         }
         output = "{\"name\" : \""+ String(fsmount) + String(entry.name())  + "\","; 
         addtos ( s, output.c_str() );
         
         if ( entry.isDirectory()  ){ 
            output = "\"type\":  2,\"files\": ";
            addtos ( s, output.c_str() );
            //Serial.printf( "%s is a directory\n", entry.name() );
            fs2json(fs,fsmount, entry.name(), s, depth+1 );
            output = tabs + "}"; addtos ( s, output.c_str() );
         } else{ 
            output = "\"type\": 3, \"size\": " + String( entry.size() ) + "}" ; 
            addtos ( s, output.c_str() );                                  
         } 
         entry.close();
}

addtos ( s, "]\n" );
dir.close();

}

//------------------------------------------------------------------------------

void listFS::allfs2json( char **s ){
  String output;

  refresh_fsd_usage();
  
  output = "{\"name\" : \"" + String(esphostname) + "\", \"type\": 0,\"filesystems\" : \n   [";
  addtos ( s, output.c_str() );
  
  bool first = true;
  for ( auto &efes : fsdlist ){           
      if ( abort_listing ) return;

      if ( first ){
        first = false; 
      }else{
        output = ",\n";addtos ( s, output.c_str() );
      }

      //The arduino String (and std::string ) class cannot automatically convert 64 bit integers
      //Good old C to the rescue.
      
      char  longlong[32];     
      output = "{\"name\": \""   + String( efes.fsname ) + "\", \"mount\": \""   + String( efes.fsmount ) + "\",\"type\": 1";
      
      
      sprintf( longlong,"%llu", efes.totalBytes );
      output += ",\"size\": " + String( longlong);

      
      sprintf( longlong,"%llu", efes.usedBytes );
      output += ",\"used\": "  + String( longlong );
      
      sprintf( longlong,"%llu", efes.freeBytes );
      output += ",\"free\": "  + String( longlong );
      output += ",\"files\" : \n";
      
      output += "      [{\"name\" : \"" + String(efes.fsmount) + "\","; 
      output += "\"type\":  2,\"files\": ";
      
      addtos ( s, output.c_str() );
      
      fs2json( *(efes.f), efes.fsmount, "/",s , 3);
      
      output = "      }]}";
      addtos ( s, output.c_str() );
      
  }
  addtos ( s, "]}\n" );
      
}

  
