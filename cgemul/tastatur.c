#include <windows.h>
#include <windowsx.h>
#include "keyboard.hpp"

HBITMAP ghbm;

BOOL OnCreate(HWND hwnd)
{
  ghbm = (HBITMAP) LoadImage (0, "./tastatur.bmp", IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);
  return TRUE;
}

BOOL OnDestroy(HWND hwnd)
{
  DeleteObject(ghbm);
  PostQuitMessage(0);
  return TRUE;
}

BOOL OnPaint(HWND hwnd)
{
  PAINTSTRUCT ps;
  HDC hdc = BeginPaint(hwnd,&ps);
  HDC hdcMem = CreateCompatibleDC(NULL);
  HBITMAP hbmT = SelectBitmap(hdcMem,ghbm);
  BITMAP bm;
  GetObject(ghbm,sizeof(bm),&bm);
  BitBlt(hdc,0,0,bm.bmWidth,bm.bmHeight,hdcMem,0,0,SRCCOPY);
  SelectBitmap(hdcMem,hbmT);
  DeleteDC(hdcMem);
  EndPaint(hwnd,&ps);
  return TRUE;
}  

void HandleButton(int x,int y);
void HandleButtonDown(int x,int y);
void HandleButtonUp();
LRESULT CALLBACK KeyboardWindowProc( HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	static int x,y;
  switch(uMsg)
  {
  case WM_CREATE:return OnCreate(hwnd);
  case WM_DESTROY:return OnDestroy(hwnd);
  case WM_PAINT:return OnPaint(hwnd);
  case WM_MOUSEMOVE:x = LOWORD(lParam);y = HIWORD(lParam);return TRUE;
  case WM_LBUTTONDOWN:HandleButtonDown(x,y);return TRUE;
  case WM_LBUTTONUP:HandleButtonUp();return TRUE;
  case WM_SIZE:return TRUE;
default:return DefWindowProc(hwnd,uMsg,wParam,lParam);
  }
  
  
}  


void KeyboardRegisterClass()
{
  WNDCLASS wc = {0,KeyboardWindowProc,0,0, NULL,
	LoadIcon(NULL,IDI_APPLICATION),
    LoadCursor(NULL,IDC_ARROW),
    (HBRUSH)(COLOR_WINDOW+1),
    NULL,
    "Keyboard"  };
  RegisterClass(&wc);
}


DWORD __stdcall KeyboardFunc(void *p)
{
	HWND hwnd;
	  MSG msg;
  KeyboardRegisterClass();
  
  hwnd = CreateWindowA("Keyboard","Keyboard",WS_OVERLAPPED,500,0,CW_USEDEFAULT,0,HWND_DESKTOP,NULL,NULL,NULL);
    
  ShowWindow(hwnd,SW_SHOW);
  SetWindowPos(hwnd,HWND_TOP,0,0,260+8,362+23+4  ,SWP_NOMOVE);
  UpdateWindow(hwnd);
  

  while(GetMessage(&msg,0,0,0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }
  return msg.wParam;
}
////////////////////////////////////////////////////////////////////////////////////////////////
#define L_OFFSET 3
#define ROWS1_WIDTH 36
#define ROWS1_GRID 43
#define ROWS1_COUNT 5
#define ROWS1_HEIGHT 22
//const int giRows1[ROWS1_COUNT]={8,47,84,121,158};
#define ROWS2_WIDTH 44
#define ROWS2_GRID 52
#define ROWS2_COUNT 4
#define ROWS2_HEIGHT 22
//const int giRows2[ROWS2_COUNT]={199,243,287,331};
#define ROWS_COUNT (ROWS1_COUNT+ROWS2_COUNT)
#define ROWS_HEIGHT 22
const int giRows[ROWS1_COUNT+ROWS2_COUNT]={8,47,84,121,158,199,243,287,331};
//extra arrows
const int giUp[2]={218,50};
const int giDown[2]={218,93};
const int giLeft[2]={189,70};
const int giRight[2]={245,70};
#define ARROW_HWIDTH (ROWS1_HEIGHT >>1)
#define ARROW_HHEIGHT (ROWS_HEIGHT >>1)
static int IsArrow(const int *pi,int x,int y)
{
	if(x<(pi[0]-ARROW_HWIDTH))return 0;
	if(x>(pi[0]+ARROW_HWIDTH))return 0;
	if(y<(pi[1]-ARROW_HHEIGHT))return 0;
	if(y>(pi[1]+ARROW_HHEIGHT))return 0;
	return 1;

}

int FindButton(int x,int y)
{
	int i,iRow=-1,iCol;

	if(IsArrow(giUp   ,x,y))return KEY_PRGM_UP;
	if(IsArrow(giDown ,x,y))return KEY_PRGM_DOWN;
	if(IsArrow(giLeft ,x,y))return KEY_PRGM_LEFT;
	if(IsArrow(giRight,x,y))return KEY_PRGM_RIGHT;
	x -= L_OFFSET;
	if(x< 0) return KEY_PRGM_NONE;
	for(i=0;i<ROWS_COUNT;i++)
	{
		if((y >= giRows[i]) && (y <= giRows[i]+ ROWS_HEIGHT))
		{
			iRow = i;
			break;
		}
	}
	if(iRow == -1)return KEY_PRGM_NONE;

	if(iRow < ROWS1_COUNT)
	{
		if((x % ROWS1_GRID) > ROWS1_WIDTH)return KEY_PRGM_NONE;
		iCol = x / ROWS1_GRID;
	}
	else
	{
	if((x % ROWS2_GRID) > ROWS2_WIDTH)return KEY_PRGM_NONE;
		iCol = x / ROWS2_GRID;
	}

	iRow = 9-iRow;
	iCol = 7 - iCol;
	i = iRow + 10*iCol;
	switch(i)
	{
	case KEY_PRGM_LEFT:
	case KEY_PRGM_RIGHT:
	case KEY_PRGM_UP:
	case KEY_PRGM_DOWN:
		return KEY_PRGM_NONE;
	case 34:
		i = KEY_PRGM_ACON;
	}

	return i;

	


}

KeyHandler(int iKey);
void HandleButton(int x,int y)
{
	int iKey = FindButton(x,y);
	if(iKey)
	{
		KeyHandler(iKey);
	}
}

int iKeyDown = 0;
void HandleButtonDown(int x,int y)
{
	iKeyDown = FindButton(x,y);

}

void HandleButtonUp()
{
	iKeyDown = 0;

}
