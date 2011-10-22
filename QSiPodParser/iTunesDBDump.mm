#include <iconv.h>

#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include <ctype.h>

int dumpiTunesDBmem( char *filename );

typedef struct {
  unsigned char pad[2];           // No NULL term!!
  unsigned char code[2];          // No NULL term!!
  unsigned long jump;
  unsigned long myLen;
  unsigned long count;
} fsbbStruct;

typedef struct {
  unsigned long res1;
  unsigned long strlength;
  unsigned long res2[2];
} stringParam;

typedef struct {
  unsigned long item_id;
  unsigned long res1[3];
} propertyParam;

typedef struct {
  unsigned long res1[2];
  unsigned long item_ref;
  unsigned short res2[2];
} playlistParam;

char dumpCode(int code)
{
  switch(code)
    {
    case 1:
      printf("Title:");
      break;
    case 2:
      printf("Filename:");
      break;
    case 3:
      printf("Album:");
      break;
    case 4:
      printf("Artist:");
      break;
    case 5:
      printf("Genre:");
      break;
    case 6:
      printf("Filetype:");
      break;
    case 7:
      printf("Unknown:");
      break;
    case 8:
      printf("Comment:");
      break;
    case 100:
      printf("Unknown:");
      break;

    default:
      return 0;
    }
  return 1;
}

int main(int argc, char** argv)
{
  if (argc != 2) {
    printf("Usage:\n");
    printf("  %s [filename]\n", argv[0]);
  } else {
    dumpiTunesDBmem(argv[1]);
  }
}

// Similar to dumpiTunesDB however, it does everything in memory
int dumpiTunesDBmem(char *filename)
{
  FILE *fp;
  int filesize;
  int filepos;
  unsigned char *contents;
  fsbbStruct fsbb;                        // First sixteen byte buffer
                
  ///////////////////////////////
  int readback;
        
  ///////////////////////////////

  fp = fopen(filename, "rb");
  if(!fp)
    return 0;
  fseek( fp, 0, SEEK_END );
  filesize = ftell( fp );
  fseek( fp, 0, SEEK_SET );

  contents = (unsigned char *) malloc(filesize);
  if(!contents) {
      fclose(fp);
      return 0;
  }

  readback = fread(contents, sizeof(unsigned char), filesize, fp);

  if(readback != filesize)
    printf("WARNING: Unable to read full file (%ld Bytes / Read %ld Bytes)\n", 
	   filesize, readback );
        
  filepos = 0;

  while(filepos < filesize) {
    // Copy a "header" of sorts
    memcpy(&fsbb, &contents[filepos], 16);
                   fsbb.jump = fsbb.myLen;  
    if (memcmp( fsbb.code, "bd", 2 ) == 0) {         // Start code
      printf("%.6x Beginning of database\n", filepos );
    } else if( memcmp( fsbb.code, "sd", 2 ) == 0 ) { // a list starter
      if( fsbb.count == 1)
	printf("%.6x List of Songs:\n", filepos );
      else if( fsbb.count == 2 )
	printf("%.6x List of Playlists:\n", filepos );
      else
	printf("%.6x Unknown (%ld)\n", filepos, fsbb.count );
    } else if( memcmp( fsbb.code, "lt", 2 ) == 0 ) { // a list starter
      printf("%.6x %ld-item list:\n", filepos, fsbb.myLen );
    } else if( memcmp( fsbb.code, "ip", 2 ) == 0 ) { // a playlist item
      playlistParam playlist;
      memcpy( &playlist, &contents[filepos+16], 16 );

      printf("%.6x itemref (%ld): XXX\n", filepos, playlist.item_ref );
    } else if( memcmp( fsbb.code, "it", 2 ) == 0 ) { // a song list item
      // item_id
      propertyParam prop;
      memcpy( &prop, &contents[filepos+16], 16 );
                        
      printf("%.6x %ld-property item (%ld):\n", filepos, fsbb.count, 
	     prop.item_id );
    } else if( memcmp( fsbb.code, "od", 2 ) == 0 ) { // usually unicode
      stringParam sp;
      iconv_t converter;
      size_t bufSize;
      size_t stringSize;
      size_t length;

      size_t charCount = 500;
      char* buf = (char*) malloc(charCount * 2);  // Usually ~100 characters?
      char* string = (char*) malloc(charCount);

      char *bufPtr = buf;
      char *stringPtr = string;

      printf("%.6x Unicode String(%c%c) %ld %ld %ld\t", filepos, fsbb.code[0], 
	     fsbb.code[1], fsbb.jump, fsbb.myLen, fsbb.count );
                        
      memcpy( &sp, &contents[filepos+16], 16 );

      if( fsbb.myLen == 0 )           // Bad something, skip outta here
	continue;

      if( sp.strlength == 0 ) {
	// printf("\tStrlength is 0\n");
	memcpy(buf, &contents[filepos+40], (fsbb.myLen-40));

	bufSize = fsbb.myLen - 40;
	stringSize = charCount;
	bufPtr = buf;
	stringPtr = string;
	converter = iconv_open("ISO_8859-1", "UNICODE");
	length = iconv(converter, &bufPtr, &bufSize, &stringPtr, &stringSize);
	iconv_close(converter);

	dumpCode( fsbb.count );
	printf(" \"%s\"\n\n", string );
      }

      fsbb.jump = fsbb.myLen;
    } else {
      printf("%.6x %c%c %ld %ld %ld\n", filepos, fsbb.code[0], fsbb.code[1], 
	     fsbb.jump, fsbb.myLen, fsbb.count );
    }

    filepos += fsbb.jump;
  }

  free( contents );
  return 1;
}
