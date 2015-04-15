// Emacs style mode select   -*- C++ -*- 
//-----------------------------------------------------------------------------
//
// $Id:$
//
// Copyright (C) 1993-1996 by id Software, Inc.
//
// This source is available for distribution and/or modification
// only under the terms of the DOOM Source Code License as
// published by id Software. All rights reserved.
//
// The source is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// FITNESS FOR A PARTICULAR PURPOSE. See the DOOM Source Code License
// for more details.
//
// $Log:$
//
// DESCRIPTION:
//
//-----------------------------------------------------------------------------



#include "os.h"

#include "doomdef.h"
#include "m_misc.h"
#include "i_video.h"

#include "g_game.h"
#include "d_main.h"

#ifdef __GNUG__
#pragma implementation "i_system.h"
#endif
#include "i_system.h"

#define RAM_I_Zone_SIZE SYSTEM_STACK_SIZE

int 			contrast_change= 0;
int 			map_mode= 0;
unsigned short 	timer_def_one;
byte			timer_def_two;
byte*			mb_mainzone;


ticcmd_t	emptycmd;
ticcmd_t*	I_BaseTiccmd(void)
{
    return &emptycmd;
}


int  I_GetHeapSize (void)
{
    return RAM_I_Zone_SIZE;
}

byte* I_ZoneBase (int*	size)
{
    *size = RAM_I_Zone_SIZE;
    mb_mainzone = (byte *) SystemStack;
    ASSERT(RAM_I_Zone_SIZE <= SYSTEM_STACK_SIZE);
    return mb_mainzone;
}

//
// I_Init
// Initialize machine state
//
void I_Init (void)
{
}


//
// I_Restore
// Restore machine state (if not done, the calculator will be killed once it enters sleep mode after the game quit)
//
void I_Restore (void)
{
}


//
// I_GetTime
// returns time in 1/70th second tics
int  I_GetTime (void)
{
    //Returns the RTC-basecount in units of 1/128 s. 
    //1/70 ~ 2/128 +-troleybus 
    return (RTC_GetTicks()/2);
}

//
// I_Quit
//
void I_Quit (void)
{
    //M_SaveDefaults ();
    fuck= true;
}


//
// toupper_int
//
int toupper_int(int i) 
{
    if(('a' <= i) && (i <= 'z'))
        return 'A' + (i - 'a');
    else
        return i;
}
//
// I_Error
//
static int key = 31;
void I_ErrorI (char *error, int i1,int i2,int i3,int i4)
{
  #ifndef CG_EMULATOR

    char buf[21];
    Bdisp_AllClr_VRAM();
    locate_OS( 1, 1 );
    PrintLine( "ERROR:", 21 );
    locate_OS( 1, 2 );
    PrintLine( (unsigned char *)error, 21 );
    locate_OS( 1, 3 );CGDAppendNum0_999("i1",i1,0,buf);PrintLine( (unsigned char *)buf, 21 );
    locate_OS( 1, 4 );CGDAppendNum0_999("i2",i2,0,buf);PrintLine( (unsigned char *)buf, 21 );
    locate_OS( 1, 5 );CGDAppendNum0_999("i3",i3,0,buf);PrintLine( (unsigned char *)buf, 21 );
    locate_OS( 1, 6 );CGDAppendNum0_999("i4",i4,0,buf);PrintLine( (unsigned char *)buf, 21 );
    {
        if(key == 31)GetKey( &key );
    }
#else
      I_Error (error,i1,i2,i3,i4);
#endif
}

void I_Error (char *error, ...)
{
#ifdef CG_EMULATOR
    /*// Message first.
    va_start (argptr,error);
    printf ("Error: ");
    printf (error,argptr);
    printf ("\n");
    sprintf(ferror,error,argptr);
    va_end (argptr);
    */
    printf ("Error: %s",error);
    //Inform the user something bad had occured
    //Exit function here
#else
  I_ShutdownGraphics();
    Bdisp_AllClr_VRAM();
    locate_OS( 1, 1 );
    PrintLine( "ERROR:", 21 );
    locate_OS( 1, 2 );
    PrintLine( (unsigned char *)error, 21 );
    {
      if(key == 31)GetKey( &key );
    }
#endif //#ifdef CG_EMULATOR
    //I_Quit();
}

//
// I_StartTic
// Processes events
// Is a bit strange because a PC handles events synchronously, while the Nspire asynchronously
//
void R_SetViewSize( int		blocks);

static void R_SetViewSizeChange(int iDiff)
{
  static int iViewSize = 10;
  iViewSize += iDiff;
  if(iViewSize >= 1 && iViewSize <= 10)
  {
    R_SetViewSize (iViewSize);
  }
  else
  {
    iViewSize -= iDiff;
  }
}

void I_StartTic (void)
{
    static event_t event = {(evtype_t)0,0,0,0};
    static int iOldKey = 0;
    int iKey = PRGM_GetKey();
    int iFilter = 1;
    switch(iKey)
    {
    case KEY_PRGM_POWER:CGCheat();break;
    case 32:R_SetViewSizeChange(-1);break;
    case 42:R_SetViewSizeChange(1);break;
    case 25:CGSwitchClip();break;
    case 65:CGFreeMem();break;
    case 58:CGRefreshSwitch();break;
        break;

    default:iFilter = 0;
    }
    if(iFilter)
    {
        while(PRGM_GetKey());
        return;
    }
    if(iKey == iOldKey)
    {
        return;
    }
    if(iOldKey)
    {
        event.type= ev_keyup;
        D_PostEvent(&event);
    }
    iOldKey = iKey;
    if(!iKey)
    {
        return;
    }
    event.type= ev_keydown;
    switch(iKey)
    {
    case KEY_PRGM_LEFT:event.data1 = KEY_LEFTARROW;break;
    case KEY_PRGM_RIGHT:event.data1 = KEY_RIGHTARROW;break;
    case KEY_PRGM_UP:event.data1 = KEY_UPARROW;break;
    case KEY_PRGM_DOWN:event.data1 = KEY_DOWNARROW;break;
    case KEY_PRGM_RETURN:event.data1 = ' ';break;
    case KEY_PRGM_ALPHA:event.data1 = KEY_RCTRL;break;
    case KEY_PRGM_MENU:I_Quit();break;
    case KEY_PRGM_F1:event.data1 = '1';break;
    case KEY_PRGM_F2:event.data1 = '2';break;
    case KEY_PRGM_F3:event.data1 = '3';break;
    case KEY_PRGM_F4:event.data1 = '4';break;
    case KEY_PRGM_F5:event.data1 = '5';break;
    case KEY_PRGM_F6:event.data1 = '6';break;
    case KEY_PRGM_SHIFT:event.data1 = '7';break;
    case KEY_PRGM_EXIT:event.data1 = KEY_TAB;break;
    case KEY_PRGM_OPTN:event.data1 = KEY_PAUSE;break;
    case 61:event.data1 = KEY_SRIGHTARROW;break;
    case 71:event.data1 = KEY_SLEFTARROW;break;
    default:return;
    }
    D_PostEvent(&event);
}
