#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

#define CG_EMULATOR

void Bdisp_PutDisp_DD();
void Bdisp_AllClr_VRAM( void );
int PRGM_GetKey();
#define WIDTH 384
#define HEIGHT 216
//extern unsigned short VRAM[WIDTH * HEIGHT];

void G3A_main( void );
int Serial_Open( unsigned char *mode );
int Serial_Close( int mode );
int Serial_DirectTransmitOneByte( unsigned char byte_to_transmit );
int Serial_CheckRX();
unsigned char Serial_Read();

#define dbgprint(x) printf(x)
#define dbgprint1(x,y) printf(x,y)


int RTC_GetTicks(  void  );
int RTC_Elapsed_ms( int start_value, int duration_in_ms );

void __stdcall Sleep(int dwMilliseconds     );
unsigned int GetVRAMAddress( void );

#define EmulHack_Sleep(x) Sleep(x)

void assert(int iLine,const char *pszFilename,const char *pszAassert);
#define ASSERT(x) if(!(x)){assert(__LINE__,__FILE__,#x);}


int Bfile_OpenFile_OS( const unsigned short*filename, int mode );
int Bfile_SeekFile_OS( int HANDLE, int pos );
int Bfile_ReadFile_OS( int HANDLE, void *buf, int size, int readpos );
int Bfile_CloseFile_OS( int HANDLE );
#define EnableColor(x)


extern const unsigned char *gpcFlashBuffer;
//flash file mapping
#define FLASH_START ((int)gpcFlashBuffer)

//page has 4 KB (I hope)
#define FLASH_PAGE_SIZE 4096
#define FLASH_PAGE_SIZE_LOG2 12
#define FLASH_PAGE_OFFSET_MASK (FLASH_PAGE_SIZE - 1)

//8K pages
#define FLASH_PAGE_COUNT (4096*2)

#define FLASH_END (FLASH_START + (FLASH_PAGE_SIZE*FLASH_PAGE_COUNT))

void InitFlashSimu( const unsigned short*filename);

