#ifndef LISTFS_H
#define LISTFS_H

class listFS : public Stream
{
public:
  listFS();
  ~listFS();
  bool open( String oname = "", const char *mode = FILE_READ); 
  const char *name(){ return( listname ); };
  void close(); 
  size_t readBytes( uint8_t *buffer, size_t length ) override;
  size_t size() { return( psbuffer_length ); };
  int available() override { return ( (int)(endpsbuffer - readpointer) ); };
  int read() override; 
  int peek() override;
  void flush() override{return;};
  size_t write( uint8_t ) override { return(0); };  
  void fs2json( fs::FS &fs, const char *fsmount, const char *dirname, char **s , int depth);
  void allfs2json( char **s );       
private:
  uint8_t  *psbuffer = NULL;
  uint8_t  *endpsbuffer = NULL;
  uint8_t  *readpointer = NULL;    
  size_t psbuffer_length=0;
  uint8_t abort_listing;
  char listname[64];

  void addtos ( char **s, const char *added);
};
extern listFS  lister;
extern listFS  directory;
extern listFS  uploadform;
extern listFS  settingsform;
extern listFS  logo;


#endif
