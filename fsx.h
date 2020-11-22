#ifndef FSX_H
#define FSX_H

//#define HTTP_UPLOAD_BUFLEN (1436 * 4)
#include <WiFi.h>
#include <WiFiClient.h>
#include <esp_wifi.h>
#include <WiFiMulti.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <esp_task_wdt.h>
#include <dirent.h>
#include <FS.h>
#include <FFat.h>
#include <SPIFFS.h>
#include <LITTLEFS.h>
#include <esp_littlefs.h>
#include <SD_MMC.h>

#include <vector>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#include <hwcrypto/aes.h>

#include <freertos/queue.h>
#include <freertos/task.h>
#include <freertos/portmacro.h>
#include <freertos/semphr.h> 

#include <wificredentials.h>

#include "listFS.h"

#define FSDSD_MMC 0
#define FSDSPIFFS 1
#define FSDLITTLEFS 2
#define FSDFFAT 3

struct fsd{
  fs::FS  *f;
  char    fsname[16];  // file system name (without trailing ':') for use with Arduino C++ interface 
  char    fsmount[16]; // topdirectoryname (with preceding '/') when using C-style file operations
  uint8_t fsnumber;    // fs number one of FSD... defines above 
  uint64_t  totalBytes;
  uint64_t  freeBytes;
  uint64_t  usedBytes;   
};

struct appconfig{
uint32_t    pauseseconds;
bool        portland;
bool        adapt_rotation;
uint8_t     portrait;
uint8_t     landscape;
};

extern struct appconfig settings;
static const char* settings_format PROGMEM="{\n\t\"portland\" : %d,\n\t\"pauseseconds\" : %d,\n \t\"adapt_rotation\" : %d,\n \t\"portrait\" : %d,\n\t\"landscape\" : %d\n}\n";
static const char* updateform PROGMEM= "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

static const char *cperror[7] PROGMEM = {"ok" ,
 "source directory not found" ,
 "source file not found" ,
 "source directory in targetdirectory" ,
 "error allocating memory for readbuffer" 
 "error reading source file" ,
 "error writing target file",
 "error creating target directory"};

 static const char uploadpage[] PROGMEM =
  R"(
<!DOCTYPE html>
<html>
<head>
 <meta charset='UTF-8'>
 <meta name='viewport' content='width=device-width, initial-scale=1.0'>

<style>
 body {
  font-family: Arial, Helvetica, sans-serif;  
  background: lightgrey;  
}
</style>
</head>
<html>
<h2>Upload file</h2>
<form method='POST' action='/upload' enctype='multipart/form-data' id='f' '>
<table>
<tr><td>Local file to upload </td><td><input type='file' name='blob' style='width: 300px;' id='pt'></td></tr>
<tr><td colspan=2> <input type='submit' value='Upload'></td></tr> 
</table>
</form>
 </html>)";


extern std::vector<struct fsd>  fsdlist;
extern AsyncWebServer           fsxserver;
extern TaskHandle_t             startwifiTask;   
extern TaskHandle_t             FSXServerTask;
extern TaskHandle_t             tftshowTask;
extern SemaphoreHandle_t updateSemaphore;
extern SemaphoreHandle_t tftSemaphore;


void    fs2json( fs::FS &fs, const char *fsmount, const char *dirname, char **s , int depth = 0);
void    allfs2json( char **s);
bool    write_list( char *readbuffer, size_t len );
void    mount_fs();
fs::FS  *fsfromfile( const char *fullfilename );
char    *nameoffs( fs::FS *ff );
size_t  fileoffset( const char *fullfilename );
bool    isdir( const char  *fullfilename );
bool    sourceintarget( String& sourcepath, String& targetpath );
int     makepath( fs::FS &ff, String &pstring );
void    maketargetpath( String& filename, String& targetdir, String& newpath );
void    deltree( fs::FS *ff, const char *path );
int     copyfile( fs::FS *sourcefs, String &sourcefspath, fs::FS *targetfs, String &targetfspath);
int     copytree( fs::FS *sourcefs, String &sourcefspath, fs::FS *targetfs, String &targetfspath);
void    startWiFi();
void    startFSXServer();
void    startTFT();

void init_config();
bool read_config();
bool write_config();


#endif
