#include "platform.h"


//CGDOOM 
#define SYSTEM_STACK_SAFE (256*1024)
#define USER_STACK_SAFE (40*1024)

//memory mapping:
//0x880A2AD5..0x880CB2D5:SaveVRAMBuffer 165888 bytes
#define SAVE_VRAM_SIZE (WIDTH*HEIGHT*2-3)
extern unsigned char *SaveVRAMBuffer;//[SAVE_VRAM_SIZE]; 

//0xA80F0000..0xA815FFFF: system stack (512 kB).
#define SYSTEM_STACK_SIZE (512*1024-SYSTEM_STACK_SAFE)
extern unsigned char *SystemStack;//[SYSTEM_STACK_SIZE]; 

extern unsigned short *VRAM;
void * CGDMalloc(int iSize);
void * CGDCalloc(int iSize);
void * CGDRealloc (void *p, int iSize);
void D_DoomMain();

void CGDAppendNum09(const char *pszText,int iNum,char *pszBuf);
int CGDstrlen(const char *pszText);
void CGDstrcpy(char *pszBuf,const char *pszText);
int CGDstrcmp (const char*s1,const char*s2);
int CGDstrncmp (const char*s1,const char*s2,int iLen);
int CGDstrnicmp (const char*s1,const char*s2,int iLen);
void CGDstrncpy(char *pszBuf,const char *pszText,int iLen);

void CGDAppendNum0_999(const char *pszText,int iNum,int iMinDigits,char *pszBuf);

int abs(int x);

extern int gWADHandle;

//force compiler error on use following:
#define strcpy 12
#define strnicmp 22
#define strncmp 27
#define strcmp 33
#define sprintf	212

//return ptr to flash
int FindInFlash(void **buf, int size, int readpos);
//direct read from flash
int Flash_ReadFile(void *buf, int size, int readpos);

//CGD: bypass for direct pointers to flash
#define PTR_TO_FLASH(x) (((int)x < FLASH_END) && ((int)x >= FLASH_START))
