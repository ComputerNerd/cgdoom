// nonstandard extension used : nameless struct/union
#pragma warning (disable : 4201)
// unreferenced inline function has been removed
#pragma warning (disable : 4514)

#define WIN32_LEAN_AND_MEAN		// Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include "emul.h"
#include <tchar.h>
#include <stdarg.h>
#include <stdio.h>
#include <windef.h>


// Global Variables:
static TCHAR *szWindowClass = _T("ConsoleW");
static HWND hWnd = NULL;
static HANDLE hThread = NULL;
static HANDLE hThreadKB = NULL;
static HANDLE hKeyLock = NULL;
static int iMainRetVal = -1;
static int ZOOM = 1;

#define KEY_BUFFER_LENGTH 16

typedef struct 
{
    int miKeyBuffer[KEY_BUFFER_LENGTH];
    int miKeyUsed;
    HANDLE mhEvent;
}GETCH_CONTEXT;


static GETCH_CONTEXT gGetchContext = {0};

static LRESULT CALLBACK	MsgHandler(HWND,UINT,WPARAM,LPARAM);
static LRESULT CALLBACK EditBoxMsgHandler(HWND,UINT,WPARAM,LPARAM);


static DWORD __stdcall ThreadProc(void *pContext);

static void ConsoleWrite(IN const char *pszText);
void KeyHandler(int iKey)
{
    /*if(iKey == VK_RETURN)
    {
        iKey = '\n';
    }*/
    WaitForSingleObject(hKeyLock,INFINITE);
	if(gGetchContext.miKeyUsed < KEY_BUFFER_LENGTH)
	{
		gGetchContext.miKeyBuffer[gGetchContext.miKeyUsed] = iKey;
		gGetchContext.miKeyUsed++;
	}
	SetEvent(gGetchContext.mhEvent);
    ReleaseMutex(hKeyLock);
}

int main(int argc,char *argv[])
{
    MSG msg;
    {
        WNDCLASS wcex = {0};
        wcex.style			= CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc	= MsgHandler;
        wcex.cbClsExtra		= 0;
        wcex.cbWndExtra		= 0;
        wcex.hInstance		= NULL;
        wcex.hIcon			= NULL;
        wcex.hCursor		= LoadCursor(NULL, IDC_ARROW); 
        wcex.hbrBackground	= (HBRUSH) GetStockObject(BLACK_BRUSH);
        wcex.lpszMenuName	= NULL;
        wcex.lpszClassName	= szWindowClass;
        RegisterClass(&wcex);
    }

    hKeyLock = CreateMutex(NULL,0,NULL);
    gGetchContext.mhEvent = CreateEvent(NULL,1,0,NULL);

	hWnd = CreateWindow(szWindowClass, "Display", WS_OVERLAPPED,CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, 0, NULL);
    if (!hWnd)
    {
        return 0;
    }
    ShowWindow(hWnd, SW_SHOW);
	SetWindowPos(hWnd,NULL,0,0,WIDTH*ZOOM+8,HEIGHT*ZOOM+23+4,SWP_NOMOVE|SWP_NOZORDER);
    UpdateWindow(hWnd);

    while (GetMessage(&msg, hWnd, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    CloseHandle(hKeyLock);
    CloseHandle(gGetchContext.mhEvent);
    CloseHandle(hThread);
	CloseHandle(hThreadKB);
    DestroyWindow(hWnd);


    return iMainRetVal;
}

unsigned short VRAM1[WIDTH * HEIGHT] = {0};
unsigned short VRAM2[WIDTH * HEIGHT] = {0};

/*#if ((sizeof(void *)) != 4)
#error "only 32 bit platform is supported"
#endif 
*/
unsigned int GetVRAMAddress( void )
{
	return (int) ((void*)VRAM1);
}

static void Redraw(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;
	int a,b,x,y,i=0;
	hDC = BeginPaint(hWnd, &ps);

	for(y=0;y<HEIGHT;y++)
	for(x=0;x<WIDTH;x++)
	{
		COLORREF c = 0;
		BYTE *bc = (BYTE*)&c;
		WORD p = VRAM1[i];

		bc[0] = ((p >> 11) & 31)<<3;
		bc[1] = ((p >>  5) & 63)<<2;
		bc[2] = ((p >>  0) & 31)<<3;
		for(a=0;a<ZOOM;a++)
		{
			for(b=0;b<ZOOM;b++)
			{
				SetPixel(hDC,x*ZOOM+b,y*ZOOM+a,c);
			}
		}

		i++;
	}

	EndPaint(hWnd, &ps);
}

static void Redraw2(HWND hWnd)
{
	PAINTSTRUCT ps;
	HDC hDC;
	int a,b,x,y,i=0;
	hDC = BeginPaint(hWnd, &ps);

	for(y=0;y<HEIGHT;y++)
	for(x=0;x<WIDTH;x++)
	{
		COLORREF c = 0;
		BYTE *bc = (BYTE*)&c;
		WORD p = VRAM1[i];
		if(p != VRAM2[i])
		{
			bc[0] = ((p >> 11) & 31)<<3;
			bc[1] = ((p >>  5) & 63)<<2;
			bc[2] = ((p >>  0) & 31)<<3;
			for(a=0;a<ZOOM;a++)
			{
				for(b=0;b<ZOOM;b++)
				{
					SetPixel(hDC,x*ZOOM+b,y*ZOOM+a,c);
				}
			}
			VRAM2[i] = p;
		}
		i++;
	}

	EndPaint(hWnd, &ps);
}

DWORD __stdcall KeyboardFunc(void *p);
static int giOptimize = 0;
static int giIsRedraw = 0;
static LRESULT CALLBACK MsgHandler(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch(message)
    {
    case WM_CREATE:
		   hThread = CreateThread(NULL,0,&ThreadProc,NULL,0,NULL);
		   hThreadKB = CreateThread(NULL,0,&KeyboardFunc,NULL,0,NULL);
        break;
    case WM_SIZE:
        break;
    case WM_CHAR:
        //KeyHandler((int)wParam);
        break;
    case WM_CLOSE:  
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
	case WM_PAINT:
		//if(giOptimize)
		Redraw2(hWnd);
		giIsRedraw = 1;
		//else
		//	Redraw(hWnd);
		//giOptimize = 0;
		break;
	case WM_RBUTTONUP://switch zoom 1x <-> 2x
		ZOOM = 3 - ZOOM;
		SetWindowPos(hWnd,NULL,0,0,WIDTH*ZOOM+8,HEIGHT*ZOOM+23+4,SWP_NOMOVE|SWP_NOZORDER);
		memset(VRAM2,0,sizeof(VRAM2));
		InvalidateRect(hWnd,NULL,FALSE);
		break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////

int Consolekbhit()
{
    return WaitForSingleObject(gGetchContext.mhEvent,0) == WAIT_OBJECT_0;
}
int Consolegetch()
{
    int iResult;
    WaitForSingleObject(gGetchContext.mhEvent,INFINITE);
    WaitForSingleObject(hKeyLock,INFINITE);
    gGetchContext.miKeyUsed--;
    iResult = gGetchContext.miKeyBuffer[gGetchContext.miKeyUsed];
    if(gGetchContext.miKeyUsed == 0)
    {
        ResetEvent(gGetchContext.mhEvent);
    }
    ReleaseMutex(hKeyLock);
    return iResult;
}

static DWORD __stdcall ThreadProc(void *pContext)
{
    G3A_main();
    PostMessage(hWnd, WM_CLOSE, 0, 0);
    return 0;
}

void Bdisp_PutDisp_DD()
{
	//Redraw2(hWnd);
	giOptimize = 1;
	giIsRedraw = 0;
	InvalidateRect(hWnd,NULL,FALSE);
	while(!giIsRedraw)Sleep(1);
}

extern int iKeyDown;
int PRGM_GetKey()
{
	/*if(Consolekbhit())
	{
		return Consolegetch();
	}*/
	return iKeyDown;
}


void Bdisp_AllClr_VRAM( void )
{
	memset(VRAM1,0,sizeof(VRAM1));
}

HANDLE ghPort = 0;
int Serial_Open( unsigned char *mode )
{
	DWORD dwMode = PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_NOWAIT;
	ghPort = CreateFileA( 
		"\\\\.\\pipe\\seriak",   // pipe name \\.\pipe\seriak
		GENERIC_READ |  // read and write access 
		GENERIC_WRITE, 
		0,              // no sharing 
		NULL,           // default security attributes
		OPEN_EXISTING,  // opens existing pipe 
		FILE_FLAG_WRITE_THROUGH, // default attributes 
		NULL);          // no template file 


	SetNamedPipeHandleState( 
		ghPort, // pipe handle 
		&dwMode,  // new pipe mode 
		NULL,     // don't set maximum bytes 
		NULL);    // don't set maximum time 
	return 0;
}

int Serial_Close( int mode )
{
	CloseHandle(ghPort);
	return 0;
}

int Serial_DirectTransmitOneByte( unsigned char byte_to_transmit )
{
	DWORD dwSent;
	WriteFile(ghPort,&byte_to_transmit,sizeof(byte_to_transmit),&dwSent,NULL);
	return 0;
}

int Serial_CheckRX()
{
	DWORD dwData = 0;
	PeekNamedPipe(ghPort,NULL,0,0,&dwData,NULL);
	//if(!dwData)Sleep(1);
	return (int)dwData;
}

unsigned char Serial_Read()
{
	DWORD dwData = 0;
	BYTE byData;
	ReadFile(ghPort,&byData,1,&dwData,NULL);
	return byData;
}


//Returns the RTC-basecount in units of 1/128 s. 
int RTC_GetTicks(  void  )
{
	return  GetTickCount()>> 3;//1000/128 is very near to 1000/125
}

//start_value has to be set with RTC_GetTicks.
//duration_in_ms is the duration, which has to elapse since start_value.
//returns 0 if duration_in_ms has not elapsed yet
//returns 1 if duration_in_ms has elapsed

int RTC_Elapsed_ms( int start_value, int duration_in_ms )
{
	return (GetTickCount() - duration_in_ms) > (start_value << 3);
}

void assert(int iLine,const char *pszFilename,const char *pszAassert)
{
	static int iDoAssert = 1;
	printf("assert %s,%i: %s\n",pszFilename,iLine,pszAassert);
	if(iDoAssert)
	{
		iDoAssert = 0;
	}
}

//files
//int hFile = Bfile_OpenFile_OS(pFileName,0);
int Bfile_OpenFile_OS( const unsigned short*filename, int mode )
{
	FILE *f;
	char szFileName[1024];
	int i = 0;
	filename += 7;//\\fls0\ 
	while(filename[0])
	{
		szFileName[i] = (char)filename[0];
		filename++;i++;
	}
	szFileName[i] = 0;
	f = fopen(szFileName,"rb");
	return f?(int)f:-1;
}

int Bfile_SeekFile_OS( int HANDLE, int pos )
{
	fseek((FILE*)HANDLE,pos,SEEK_SET);
	return 0;
}

int Bfile_ReadFile_OS( int HANDLE, void *buf, int size, int readpos )
{
  if(readpos != -1)
  {
    Bfile_SeekFile_OS(HANDLE, readpos );
  }
  //fread ( dest, size elements, count elements, FILE handle );
  return fread(buf,1,size,(FILE*)HANDLE);
}

int Bfile_CloseFile_OS( int HANDLE )
{
	fclose((FILE*)HANDLE);
	return 0;
}

static unsigned char gcFlashBuffer[FLASH_PAGE_SIZE * FLASH_PAGE_COUNT] = {0};
const unsigned char *gpcFlashBuffer = gcFlashBuffer;

static const char *gpszFlashFileName = "flash.bin";
static void CreateFlash( const unsigned short*filename)
{
  int f = Bfile_OpenFile_OS(filename,0);
  int iRet;
  int iIndex = 0;

  for(;;)
  {
 //   iIndex++;

    iRet = Bfile_ReadFile_OS(f,gcFlashBuffer + (iIndex * FLASH_PAGE_SIZE), FLASH_PAGE_SIZE,-1);
    iIndex++;
    if(iRet < FLASH_PAGE_SIZE)
    {
      break;
    }
  }
  Bfile_CloseFile_OS(f);
  {
    FILE *f = fopen(gpszFlashFileName,"wb");
    fwrite(gcFlashBuffer,1,sizeof(gcFlashBuffer),f);
    fclose(f);
  }
}

void InitFlashSimu( const unsigned short*filename)
{
  DWORD dwFileSize;
  HANDLE hMap;
  HANDLE hFile=CreateFileA(gpszFlashFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
  if(hFile==INVALID_HANDLE_VALUE)
  {
    CreateFlash( filename);
    hFile=CreateFileA(gpszFlashFileName,GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
  };
  ASSERT(hFile!=INVALID_HANDLE_VALUE);

  // create anonymous file mapping
  hMap=CreateFileMapping(hFile,NULL,PAGE_READWRITE,0,0,NULL);
  ASSERT(hMap!=NULL);

  dwFileSize=GetFileSize(hFile,NULL); // get file size

  // map buffer to local process space
  gpcFlashBuffer =(unsigned char *)MapViewOfFile(hMap,FILE_MAP_READ,0,0,dwFileSize);
  ASSERT(gpcFlashBuffer!=NULL);
}