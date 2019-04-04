
#include "platform.h"
#include "os.h"

#ifdef CG_EMULATOR
static int iAllocSum = 0;
#endif 
void * CGDMalloc(int iSize)
{
#ifdef CG_EMULATOR

	iAllocSum += iSize;
	printf("malloc %i (%i)\n",iSize,iAllocSum);
#endif 
	return malloc(iSize);
}

void * CGDCalloc(int iSize)
{
	void *p = CGDMalloc(iSize);
	if(p != NULL)
	{
		memset(p,0,iSize);
	}
	return p;
}


void * CGDRealloc (void *p, int iSize)
{
#ifdef CG_EMULATOR
	iAllocSum += iSize;
	printf("realloc %i (%i)\n",iSize,iAllocSum);
#endif 
	if(p == NULL)
	{
		return malloc(iSize);
	}
	else
	{
		return realloc(p,iSize);
	}
}


#ifdef CG_EMULATOR
unsigned char aSaveVRAMBuffer[SAVE_VRAM_SIZE]; //for screens[2] (2x64KB) + WAD file mapping
unsigned char aSystemStack[SYSTEM_STACK_SIZE]; //for RAM_I_Zone
#else
unsigned char __attribute__ ((aligned (4))) aSystemStack[SYSTEM_STACK_SIZE]; //for RAM_I_Zone
#endif 
unsigned short *VRAM;
unsigned char *SaveVRAMBuffer; //for screens[2]
unsigned char *SystemStack; //for RAM_I_Zone


void CGDAppendNum09(const char *pszText,int iNum,char *pszBuf)
{
	int i = 0;
	while(pszText[i])
	{
		pszBuf[i] = pszText[i];
		i++;
	}
	ASSERT(iNum < 10);
	ASSERT(iNum >= 0);
	pszBuf[i] = (char)('0'+iNum);
	pszBuf[i+1] = 0;
}

void CGDAppendNum0_999(const char *pszText,int iNum,int iMinDigits,char *pszBuf)
{
	int i = 0;
	int z = 0;
	while(pszText[i])
	{
		pszBuf[i] = pszText[i];
		i++;
	}
	ASSERT(iNum < 1000000);
	ASSERT(iNum >= 0);
	if((iNum > 999999) || (iMinDigits>6))
	{
		pszBuf[i] = (char)('0'+(iNum / 1000000));
		iNum %= 1000000;
		i++;
		z = 1;
	}
	if(z || (iNum > 99990) || (iMinDigits>5))
	{
		pszBuf[i] = (char)('0'+(iNum / 100000));
		iNum %= 100000;
		i++;
		z = 1;
	}

	if(z || (iNum > 9999) || (iMinDigits>4))
	{
		pszBuf[i] = (char)('0'+(iNum / 10000));
		iNum %= 10000;
		i++;
		z = 1;
	}
	if(z || (iNum > 999) || (iMinDigits>3))
	{
		pszBuf[i] = (char)('0'+(iNum / 1000));
		iNum %= 1000;
		i++;
		z = 1;
	}
	if(z || (iNum > 99) || (iMinDigits>2))
	{
		pszBuf[i] = (char)('0'+(iNum / 100));
		iNum %= 100;
		i++;
		z = 1;
	}
	if(z || (iNum > 9) || (iMinDigits>1))
	{
		pszBuf[i] = (char)('0'+(iNum / 10));
		iNum %= 10;
		i++;
	}
	pszBuf[i] = (char)('0'+iNum);
	pszBuf[i+1] = 0;
}


int CGDstrlen(const char *pszText)
{
	int i = 0;
	while(pszText[i])
	{
		i++;
	}
	return i;
}

void CGDstrcpy(char *pszBuf,const char *pszText)
{
	int i = 0;
	while(pszText[i])
	{
		pszBuf[i] = pszText[i];
		i++;
	}
	pszBuf[i] = 0;
}

void CGDstrncpy(char *pszBuf,const char *pszText,int iLen)
{
	int i = 0;
	while(pszText[i])
	{
		if(iLen == i)
		{
			return;
		}
		pszBuf[i] = pszText[i];
		i++;
	}
	pszBuf[i] = 0;
}


int CGDstrcmp (const char*s1,const char*s2)
{
	while(s1[0])
	{
		unsigned char c1 = (unsigned char)s1[0];
		unsigned char c2 = (unsigned char)s2[0];
		if(c1 !=c2)
		{
			return 1;//who cares 1/-1, important is match/nonmatch
		}
		s1++;
		s2++;
	}
	return (unsigned char)s2[0]>0;
}
/*int CGDstrcmp (const char*s1,const char*s2)
{
int i = CGDstrcmpX(s1,s2);
ASSERT(!i==!strcmp(s1,s2));
return i;
}*/


int CGDstrncmp (const char*s1,const char*s2,int iLen)
{
	if(!iLen)
	{
		return 0;
	}

	while(s1[0])
	{
		unsigned char c1 = (unsigned char)s1[0];
		unsigned char c2 = (unsigned char)s2[0];

		if(c1 !=c2)
		{
			return 1;//who cares 1/-1, important is match/nonmatch
		}
		s1++;
		s2++;

		iLen--;
		if(!iLen)
		{
			return 0;
		}
	}
	return (unsigned char)s2[0]>0;
}
/*int CGDstrncmp (const char*s1,const char*s2,int iLen)
{
int i = CGDstrncmpX(s1,s2,iLen);
ASSERT(!i==!strncmp(s1,s2,iLen));
return i;
}*/

static unsigned char Upper(unsigned char c)
{
	if((c>='a')&&(c<='z'))
	{
		c -='a'-'A';
	}
	return c;
}

int CGDstrnicmp (const char*s1,const char*s2,int iLen)
{
	if(!iLen)
	{
		return 0;
	}

	while(s1[0])
	{
		unsigned char c1 = Upper((unsigned char)s1[0]);
		unsigned char c2 = Upper((unsigned char)s2[0]);

		if(c1 !=c2)
		{
			return 1;//who cares 1/-1, important is match/nonmatch
		}
		s1++;
		s2++;
		iLen--;
		if(!iLen)
		{
			return 0;
		}
	}
	return (unsigned char)s2[0]>0;
}

/*int CGDstrnicmp (const char*s1,const char*s2,int iLen)
{
int i = CGDstrnicmpX(s1,s2,iLen);
int j = strnicmp(s1,s2,iLen);
ASSERT(!i==!j);
return i;
}
*/

int gWADHandle;
const unsigned short wadfile[] = {'\\','\\','f','l','s','0','\\','d','o','o','m','.','w','a','d',0};

////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////
//The whole sound doesn't fir onto the RAM.
//Reading per partes is not possible as this is synchronnous player (there would be silences when reading).
//So I read each page (4KB)of the wav file and try to find it in the flash. 
//Simply finding start of the file is not enough because of fragmentation.

//Seach the whole flash, do not assume FS start at 0xA1000000 
//(already tried interval 0xA1000000 - 0xA1FFFFFF, but some parts of the file were outside of this interval)
// move this to platform.h, simulator will use static buffer
//#define FLASH_START 0xA0000000	
//page has 4 KB (I hope)
//#define FLASH_PAGE_SIZE 4096
//8K pages
//#define FLASH_PAGE_COUNT (4096*2)

//allocate 1024 items for max 1024 fragments of the file.
// 640 KB should to be enough for everyone ;-)
#define MAX_FRAGMENTS 1024
//descriptor for 1 fragment
typedef struct
{
	short msOffset;//page index (0 ~ 8K)
	short msCount;//count of pages in this fragment
}FileMappingItem;


typedef struct
{
	FileMappingItem mTable[MAX_FRAGMENTS];//table of fragments
	int miItemCount;
	int miTotalLength;//length of the file
	int miCurrentLength;//currently returned length (by GetNextdata() )
	int miCurrentItem;//active fragment (to be returned by GetNextdata() )

}FileMapping;

//reset reading to start
void ResetData(FileMapping *pMap)
{
	pMap->miCurrentItem = 0;
	pMap->miCurrentLength = 0;
}


static int memcmp32(const int *p1,const int *p2,int count)
{
	while(count)
	{
		if(*p1 != *p2)
		{
			return 1;
		}
		p1++;
		p2++;
		count--;
	}
	return 0;
}

//I was able to use memcmp(), it compiled but it seems it returned always 0 
// quick fix fot now
/*int Xmemcmp(const char *p1,const char *p2,int len)
{
	if(!(len & 3))
	{
		return memcmp32((int*)p1,(int*)p2,len >>2);
	}
	while(len)
	{
		if(*p1 != *p2)
		{
			return 1;
		}
		p1++;
		p2++;
		len--;
	}
	return 0;
}*/
#define Xmemcmp memcmp

int CreateFileMapping(const unsigned short *pFileName,FileMapping *pMap){
	int iResult = 0;
	char cBuffer[FLASH_PAGE_SIZE];
	int hFile = Bfile_OpenFile_OS(pFileName,0,0);
	int iLength;
	char *pFlashFS = (char *)FLASH_START;

	pMap->miItemCount = 0;
	pMap->miTotalLength = 0;
	iLength = Bfile_ReadFile_OS(hFile,cBuffer,FLASH_PAGE_SIZE,-1);
	while(iLength > 0)
	{
		//do not optimize (= do not move these 2 variables before loop)!
		// fx-cg allocates pages for file in <random> order so page from the end of the file 
		//can have lower index than page from the beginning
		const char *pTgt = pFlashFS;
		int iPageIndx = 0;

		for(;iPageIndx < FLASH_PAGE_COUNT;iPageIndx++)
		{
			if(!Xmemcmp(pTgt,cBuffer,iLength))
			{
				break;
			}
			pTgt += FLASH_PAGE_SIZE;
		}
		if(iPageIndx == FLASH_PAGE_COUNT)
		{
			//page not found !
			iResult = -2;
			goto lbExit;
		}
		pMap->miItemCount ++;
		if(pMap->miItemCount >= MAX_FRAGMENTS)
		{
			//file too fragmented !
			iResult = -3;
			goto lbExit;
		}
		pMap->mTable[pMap->miItemCount-1].msOffset = (short)iPageIndx;
		pMap->mTable[pMap->miItemCount-1].msCount = 0;
		//assume fragment has more pages
		for(;;)
		{
			pMap->mTable[pMap->miItemCount-1].msCount++;
			pMap->miTotalLength += iLength;
			iPageIndx++;
			pTgt += FLASH_PAGE_SIZE;

			if(iLength < FLASH_PAGE_SIZE)
			{
				//this was the last page
				iResult = pMap->miTotalLength;
				goto lbExit;
			}
			iLength = Bfile_ReadFile_OS(hFile,cBuffer,FLASH_PAGE_SIZE,-1);
			if(iLength <= 0)
			{
				break;
			}
			if(Xmemcmp(pTgt,cBuffer,iLength))
			{
				break;
			}
		}
	}
	if(iLength < 0)
	{
		iResult = -1;
	}
	else
	{
		if(pMap->miTotalLength >50000)
		{
			pMap->miTotalLength = 50000;//hack
		}

		iResult = pMap->miTotalLength;
	}

lbExit:
	Bfile_CloseFile_OS(hFile);
	return iResult;

}
void I_Error (char *error, ...);
static FileMapping *gpWADMap = 0;

int FindInFlash(void **buf, int size, int readpos)
{
	int iPageReq = readpos >>FLASH_PAGE_SIZE_LOG2;
	int iPageIndx = 0;
	int iCurrOffset = 0, iCurrLen;
	int iSubOffset;
	ASSERT(readpos >=0);
	//find item
	for(;;)
	{
		if(iPageIndx >= gpWADMap->miItemCount)
		{
			return -1;
		}
		if(iPageReq < gpWADMap->mTable[iPageIndx].msCount)
		{
			ASSERT(iCurrOffset <= readpos);
			break;
		}
		iPageReq -= gpWADMap->mTable[iPageIndx].msCount;
		iCurrOffset += ((int)gpWADMap->mTable[iPageIndx].msCount) << FLASH_PAGE_SIZE_LOG2;
		iPageIndx++;
	}
	iSubOffset = readpos - iCurrOffset;
	iCurrLen = (gpWADMap->mTable[iPageIndx].msCount * FLASH_PAGE_SIZE) - iSubOffset;
	ASSERT(iCurrLen > 0);
	if(iCurrLen > size)
	{
		iCurrLen = size;
	}
	*buf = ((char *)FLASH_START)+(gpWADMap->mTable[iPageIndx].msOffset << FLASH_PAGE_SIZE_LOG2)+iSubOffset;
	return iCurrLen;
}

int Flash_ReadFile(void *buf, int size, int readpos)
{
	void *pSrc;
	int iRet = 0;
	while(size >0)
	{
		int i = FindInFlash(&pSrc,size, readpos);
		if(i<0)
			return i;
		memcpy(buf,pSrc,i);
		buf = ((char*)buf)+i;
		readpos +=i;
		size -=i;
		iRet +=i;
	}
	return iRet;
}


int Flash_ReadFileX(void *buf, int size, int readpos)
{
	int iRet = 0;
	if(gpWADMap->miTotalLength - readpos < size)
	{
		size = gpWADMap->miTotalLength - readpos;
	}
	/*
	typedef struct
	{
	short msOffset;//page index (0 ~ 8K)
	short msCount;//count of pages in this fragment
	}FileMappingItem;

	typedef struct
	{
	FileMappingItem mTable[MAX_FRAGMENTS];//table of fragments
	int miItemCount;
	int miTotalLength;//length of the file

	}FileMapping;*/
	ASSERT(readpos >=0);
	ASSERT(FLASH_PAGE_SIZE == 1<< FLASH_PAGE_SIZE_LOG2);
	for(;;)
	{
		int iPageReq = readpos >>FLASH_PAGE_SIZE_LOG2;
		int iPageIndx = 0;
		int iCurrOffset = 0, iCurrLen;
		int iSubOffset;
		//find item
		for(;;)
		{
			if(iPageIndx >= gpWADMap->miItemCount)
			{
				return -1;
			}
			if(iPageReq < gpWADMap->mTable[iPageIndx].msCount)
			{
				ASSERT(iCurrOffset <= readpos);
				break;
			}
			iPageReq -= gpWADMap->mTable[iPageIndx].msCount;
			iCurrOffset += ((int)gpWADMap->mTable[iPageIndx].msCount) << FLASH_PAGE_SIZE_LOG2;
			iPageIndx++;
		}
		iSubOffset = readpos - iCurrOffset;
		iCurrLen = (gpWADMap->mTable[iPageIndx].msCount * FLASH_PAGE_SIZE) - iSubOffset;
		ASSERT(iCurrLen > 0);
		if(iCurrLen > size)
		{
			iCurrLen = size;
		}
		memcpy(buf,((char *)FLASH_START)+(gpWADMap->mTable[iPageIndx].msOffset << FLASH_PAGE_SIZE_LOG2)+iSubOffset,iCurrLen);
		readpos += iCurrLen;
		size -= iCurrLen;
		buf = ((char *)buf) +iCurrLen;
		iRet += iCurrLen;
		if(size ==0)
		{
			break;
		}
	}
	return iRet;
}

void abort(void){
	int x=0,y=160;
	PrintMini(&x,&y,"Abort called",0,0xFFFFFFFF,0,0,0xFFFF,0,1,0);
	int key;
	for(;;)
		GetKey(&key);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
void main(void){
	InitFlashSimu(wadfile); //load wad file to flash simulation on simulator, do nothing on real HW
#ifdef CG_EMULATOR
	SaveVRAMBuffer = aSaveVRAMBuffer;
#else 
	unsigned tmp=((unsigned)getSecondaryVramAddress()+3)&(~3);
	SaveVRAMBuffer = (unsigned char*)tmp;
#endif 
	SystemStack = aSystemStack;
	VRAM = (unsigned short*)GetVRAMAddress();
	EnableColor(1);
	memset(VRAM,0,WIDTH*HEIGHT*2);
	gpWADMap = (FileMapping *)(SaveVRAMBuffer + 2*65536);
	ASSERT(2*65536 + sizeof(FileMapping) < SAVE_VRAM_SIZE);
	gWADHandle =	CreateFileMapping(wadfile,gpWADMap);
	switch(gWADHandle)
	{
	case -1:
		I_Error ("File read error");
		return;
		break;
	case -2:
		I_Error ("Page not found");
		return;
		break;
	case -3:
		I_Error ("File too fragmented");
		return;
		break;
	default:
		break;
	}
	D_DoomMain(); 
}

//todo: wrapper pro (patch_t*), + flash
