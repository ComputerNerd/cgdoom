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
// DOOM main program (D_DoomMain) and game loop (D_DoomLoop),
// plus functions to determine game mode (shareware, registered),
// parse command line parameters, configure game parameters (turbo),
// and call the startup functions.
//
//-----------------------------------------------------------------------------


#define BGCOLOR  7
#define FGCOLOR  8


#include "os.h"

#include "doomdef.h"
#include "doomstat.h"

#include "dstrings.h"


#include "z_zone.h"
#include "w_wad.h"
#include "v_video.h"

#include "f_finale.h"
#include "f_wipe.h"

#include "m_misc.h"
#include "m_menu.h"

#include "i_system.h"
#include "i_video.h"

#include "g_game.h"

#include "hu_stuff.h"
#include "wi_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

#include "p_setup.h"
#include "r_local.h"


#include "d_main.h"


//
// D-DoomLoop()
// Not a globally visible function,
//  just included for source reference,
//  called by D_DoomMain, never exits.
// Manages timing and IO,
//  calls all ?_Responder, ?_Ticker, and ?_Drawer,
//  calls I_GetTime, I_StartFrame, and I_StartTic
//
void D_DoomLoop (void);


boolean   devparm; // started game with -devparm
boolean         nomonsters; // checkparm of -nomonsters
boolean         respawnparm; // checkparm of -respawn
boolean         fastparm; // checkparm of -fast

boolean         drone;

boolean   singletics = true;

boolean   fuck = false;


extern boolean inhelpscreens;

skill_t   startskill;
int    startepisode;
int    startmap;
boolean   autostart;

boolean   advancedemo;
void D_ProcessEvents (void);
void G_BuildTiccmd (ticcmd_t* cmd);
void D_DoAdvanceDemo (void);

//
// D_PostEvent
// Called by the I/O functions when input is detected
//
void D_PostEvent (event_t* ev)
{
  if (M_Responder (ev))
    return;               // menu ate the event
  G_Responder (ev);
}

//
// D_Display
//  draw current display, possibly wiping it from the previous
//

// wipegamestate can be set to -1 to force a wipe on the next draw
gamestate_t     wipegamestate = GS_DEMOSCREEN;
extern boolean setsizeneeded;
extern int  showMessages;
void    R_ExecuteSetViewSize (void);

void D_Display (void)
{
  static  boolean   viewactivestate = false;
  static  boolean   menuactivestate = false;
  static  boolean   inhelpscreensstate = false;
  static  boolean   fullscreen = false;
  static  gamestate_t  oldgamestate = (gamestate_t)-1;
  static  int    borderdrawcount;
  int      nowtime;
  int      tics;
  int      wipestart;
  int      y;
  boolean     done;
  boolean     wipe;
  boolean     redrawsbar;

  redrawsbar = false;

  // change the view size if needed
  if (setsizeneeded)
  {
    R_ExecuteSetViewSize ();
    oldgamestate = (gamestate_t)-1;                      // force background redraw
    borderdrawcount = 3;
  }

  // save the current screen if about to wipe
  if (gamestate != wipegamestate)
  {
    wipe = true;
    wipe_StartScreen();
  }
  else
    wipe = false;

  if (gamestate == GS_LEVEL && gametic)
    HU_Erase();

  // do buffered drawing
  switch (gamestate)
  {
  case GS_LEVEL:
    if (!gametic)
      break;
    if (automapactive)
      AM_Drawer ();
    if (wipe || (viewheight != 200 && fullscreen) )
      redrawsbar = true;
    if (inhelpscreensstate && !inhelpscreens)
      redrawsbar = true;              // just put away the help screen
    ST_Drawer ((boolean)(viewheight == 200), redrawsbar );
    fullscreen = (boolean)(viewheight == 200);
    break;

  case GS_INTERMISSION:
    WI_Drawer ();
    break;

  case GS_FINALE:
    F_Drawer ();
    break;

  case GS_DEMOSCREEN:
    D_PageDrawer ();
    break;
  }

  // draw the view directly
  if (gamestate == GS_LEVEL && !automapactive && gametic)
    R_RenderPlayerView (&players[0]);

  if (gamestate == GS_LEVEL && gametic)
    HU_Drawer ();

  menuactivestate = menuactive;
  viewactivestate = viewactive;
  inhelpscreensstate = inhelpscreens;
  oldgamestate = gamestate;

  // draw pause pic
  if (paused)
  {
    if (automapactive)
      y = 4;
    else
      y = viewwindowy+4;
    V_DrawPatchDirect(viewwindowx+(scaledviewwidth-68)/2, y ,0 ,W_CacheLumpNamePatch("M_PAUSE", PU_CACHE) );
  }

  // menus go directly to the screen
  M_Drawer ();          // menu is drawn even on top of everything

  // normal update
  if (!wipe)
  {
    I_Flip ();              // page flip or blit buffer
    return;
  }

  // wipe update
  wipe_EndScreen(0, 0, SCREENWIDTH, SCREENHEIGHT);

  wipestart = I_GetTime () - 1;

  do
  {
    do
    {
      nowtime = I_GetTime ();
      tics = nowtime - wipestart;
    } while (tics<=0);
    wipestart = nowtime;
    done = wipe_ScreenWipe(wipe_Melt, SCREENWIDTH, SCREENHEIGHT, tics);
    M_Drawer ();       // menu is drawn even on top of wipes
    I_Flip ();        // page flip or blit buffer
  } while (!done);
  gamestate = oldgamestate;
  wipegamestate= oldgamestate;
}



//
//  D_DoomLoop
//
extern  boolean  demorecording;
extern int giRefreshMask;
void D_DoomLoop (void)
{
  I_InitGraphics ();

  while (!fuck)
  {
    I_StartTic ();

    if (advancedemo)
      D_DoAdvanceDemo ();

    M_Ticker (); //Menu ticker

    G_Ticker (); //Game ticker

    gametic++;

    // Update display, next frame, with current state
    if(!(gametic & giRefreshMask))
    {
      D_Display ();
    }

  }
 // I_ShutdownGraphics();
  free(lumpinfo);
  free(lumpcache);
  I_ShutdownGraphics();
  return;
}



//
//  DEMO LOOP
//
int             demosequence;
int             pagetic;
char           *pagename;


//
// D_PageTicker
// Handles timing for warped projection
//
void D_PageTicker (void)
{
  if (--pagetic < 0)
    D_AdvanceDemo ();
}



//
// D_PageDrawer
//
void D_PageDrawer (void)
{
  V_DrawPatch (0,0, 0, (const patch_t *)W_CacheLumpNameConst(pagename, PU_CACHE));
}


//
// D_AdvanceDemo
// Called after each demo or intro demosequence finishes
//
void D_AdvanceDemo (void)
{
  advancedemo = true;
}

int episode = 1;
int map = 1;    //HACK
int skill = 2;

//
// This cycles through the demo sequences.
// FIXME - version dependend demo numbers?
//
void D_DoAdvanceDemo (void)
{
  pagename = "TITLEPIC";
  gamestate= GS_LEVEL;
  G_DeferedInitNew((skill_t)skill,episode,map);
  advancedemo= false;
}



//
// D_StartTitle
//
void D_StartTitle (void)
{
  gameaction = ga_nothing;
  demosequence = -1;
  D_AdvanceDemo ();
}




//      print title for every printed line
//char            title[128];

//
// D_DoomMain
//
void D_DoomMain()
{
  int ok_wad=0;
//  int selected=1;
//  int rskill=1;
  int maxepisode=1;
  skill=3;

//  int repisode=1;
//  int rmap=1;

  ok_wad=W_InitMultipleFiles();

  if(W_CheckNumForName("MAP01")!=-1)
    gamemode=commercial;
  else if(W_CheckNumForName("E1M1")==-1)
    gamemode=indetermined;
  else if (W_CheckNumForName("E2M1")==-1)
    gamemode=shareware;
  else if (W_CheckNumForName("E4M1")==-1)
    gamemode=registered;
  else
    gamemode=retail;  

  if(gamemode == indetermined) // CX port
    return; // CX port

  {//save sprintf
    if (W_CheckNumForName("E2M1")!=-1)
    {
      maxepisode = 2;
      if (W_CheckNumForName("E3M1")!=-1)
        maxepisode = 3;
    }
#if 0 //CG:seem to be useless
    while(true)
    { if(gamemode==commercial)
    sprintf(res,"MAP%02i",maxmap+1);
    else
      sprintf(res,"E1M%i",maxmap+1);
    if (W_CheckNumForName(res)==-1)
      break;
    maxmap++;
    }
#endif 
  }

  if(maxepisode>1)
#if 0 //TODO !
    do
    {
      if(maxepisode>1)
      { if(isKeyPressed(KEY_NSPIRE_RP) && episode<maxepisode) { episode++; repisode=1; while(isKeyPressed(KEY_NSPIRE_RP)); }
      if(isKeyPressed(KEY_NSPIRE_LP) && episode>1)  { episode--; repisode=1; while(isKeyPressed(KEY_NSPIRE_LP)); }
      if(repisode)
      { sprintf(temp,"%i",episode);
      drwBufStr(SCREEN_BASE_ADDRESS, 13*CHAR_WIDTH,  14*CHAR_HEIGHT, temp, 0, 0);
      repisode = 0;
      }
      }
      if(isKeyPressed(KEY_NSPIRE_PLUS) && skill<4) { skill++; rskill=1; while(isKeyPressed(KEY_NSPIRE_PLUS)); }
      if(isKeyPressed(KEY_NSPIRE_MINUS) && skill>0)  { skill--; rskill=1; while(isKeyPressed(KEY_NSPIRE_MINUS)); }
      if(rskill)
      { if(skill==0) tmp="I'm too young to die";
      else if(skill==1) tmp="  Hey, not to rough ";
      else if(skill==2) tmp="   Hurt me plenty   ";
      else if(skill==3) tmp="   Ultra-Violence   ";
      else if(skill==4) tmp="     Nightmare!     ";
      drwBufStr(SCREEN_BASE_ADDRESS, 11*CHAR_WIDTH, 11*CHAR_HEIGHT, tmp, 0, 0);
      rskill = 0;
      }
    }
    while(!isKeyPressed(KEY_NSPIRE_ESC) && !isKeyPressed(KEY_NSPIRE_ENTER) && !isKeyPressed(KEY_NSPIRE_RET) && !isKeyPressed(KEY_NSPIRE_CLICK));
#endif 

    //if(isKeyPressed(KEY_NSPIRE_ESC)) // CX port
    //return; // CX port

    nomonsters = 0;
    respawnparm = 0;
    fastparm = 0;
    devparm = 0;

    // get skill / episode / map from parms // DON'T, LEFT IN UNTIL OPCHECK
    if(skill==0)  startskill = sk_baby;
    else if(skill==1) startskill = sk_easy;
    else if(skill==2) startskill = sk_medium;
    else if(skill==3) startskill = sk_hard;
    else if(skill==4) startskill = sk_nightmare;

    startepisode = episode;
    startmap = map;
    autostart = false;

    // init subsystems
    //printf ("V_Init: Allocate screens\n");
    V_Init ();
    //printf ("Z_Init: Init zone memory allocation daemon \n");
    Z_Init ();
    //printf ("W_Init: Init WADfiles\n");

    W_InitMultipleFiles ();
    //Just in case we haven't found the WAD files...
    if (fuck)
      return;

#ifdef CG_EMULATOR
    // Check and print which version is executed.
    switch ( gamemode )
    {
    case shareware:
    case indetermined:
      printf (
        "=====================================\n"
        "             Shareware!\n"
        "=====================================\n"
        );
      break;
    case registered:
    case retail:
    case commercial:
      printf (
        "=====================================\n"
        "          Commercial product\n"
        "          do not distribute!\n"
        "=====================================\n"
        );
      break;

    default:
      // Ouch.
      break;
    }
#endif //#ifdef CG_EMULATOR
    //printf ("I_Init: Setting up machine state\n");
    I_Init ();
    //printf ("M_Init: Init miscellaneous info\n");
    M_Init ();
    //printf ("R_Init: Init DOOM refresh daemon \n");
    R_Init ();
    //printf ("P_Init: Init Playloop state\n");
    P_Init ();
    //printf ("HU_Init: Setup heads up display\n");
    HU_Init ();
    //printf ("ST_Init: Init status bar\n");
    ST_Init ();
    //printf ("Engage... \n");
    D_StartTitle ();
    D_DoomLoop ();  // never returns, except when it does
    I_Restore (); //calckill = bad

    return;
}
