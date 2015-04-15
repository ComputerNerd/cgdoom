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
// Game completion, final screen animation.
//
//-----------------------------------------------------------------------------


//static const char 


#include "os.h"

// Functions.
#include "i_system.h"
#include "m_swap.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

// Data.
#include "dstrings.h"

#include "doomstat.h"
#include "r_state.h"

// ?
//#include "doomstat.h"
//#include "r_local.h"
//#include "f_finale.h"

// Stage of animation:
//  0 = text, 1 = art screen, 2 = character cast
int  finalestage;

int  finalecount;

#define TEXTSPEED 3
#define TEXTWAIT 250

const char* e1text = E1TEXT;
const char* e2text = E2TEXT;
const char* e3text = E3TEXT;
const char* e4text = E4TEXT;

const char* c1text = C1TEXT;
const char* c2text = C2TEXT;
const char* c3text = C3TEXT;
const char* c4text = C4TEXT;
const char* c5text = C5TEXT;
const char* c6text = C6TEXT;

const char* p1text = P1TEXT;
const char* p2text = P2TEXT;
const char* p3text = P3TEXT;
const char* p4text = P4TEXT;
const char* p5text = P5TEXT;
const char* p6text = P6TEXT;

const char* t1text = T1TEXT;
const char* t2text = T2TEXT;
const char* t3text = T3TEXT;
const char* t4text = T4TEXT;
const char* t5text = T5TEXT;
const char* t6text = T6TEXT;

const char* finaletext;
char* finaleflat;

void F_StartCast (void);
void F_CastTicker (void);
boolean F_CastResponder (event_t *ev);
void F_CastDrawer (void);

//
// F_StartFinale
//
void F_StartFinale (void)
{
  gameaction = ga_nothing;
  gamestate = GS_FINALE;
  viewactive = false;
  automapactive = false;

  // Okay - IWAD dependent stuff.
  // This has been changed severly, and
  //  some stuff might have changed in the process.
  switch ( gamemode )
  {
    // DOOM 1 - E1, E3 or E4, but each nine missions
  case shareware:
  case registered:
  case retail:
    {
      switch (gameepisode)
      {
      case 1:
        finaleflat = "FLOOR4_8";
        finaletext = e1text;
        break;
      case 2:
        finaleflat = "SFLR6_1";
        finaletext = e2text;
        break;
      case 3:
        finaleflat = "MFLR8_4";
        finaletext = e3text;
        break;
      case 4:
        finaleflat = "MFLR8_3";
        finaletext = e4text;
        break;
      default:
        // Ouch.
        break;
      }
      break;
    }

    // DOOM II and missions packs with E1, M34
  case commercial:
    {
      switch (gamemap)
      {
      case 6:
        finaleflat = "SLIME16";
        finaletext = c1text;
        break;
      case 11:
        finaleflat = "RROCK14";
        finaletext = c2text;
        break;
      case 20:
        finaleflat = "RROCK07";
        finaletext = c3text;
        break;
      case 30:
        finaleflat = "RROCK17";
        finaletext = c4text;
        break;
      case 15:
        finaleflat = "RROCK13";
        finaletext = c5text;
        break;
      case 31:
        finaleflat = "RROCK19";
        finaletext = c6text;
        break;
      default:
        // Ouch.
        break;
      }
      break;
    }

    // Indeterminate game
  default:
    finaleflat = "F_SKY1"; // Not used anywhere else.
    finaletext = c1text;  // Displays a 'FIXME' warning message
    break;
  }

  finalestage = 0;
  finalecount = 0;

}



boolean F_Responder (event_t *event)
{
  if (finalestage == 2)
    return F_CastResponder (event);
  else
    return false;
}


//
// F_Ticker
//
void F_Ticker (void)
{
  int  i;

  // check for skipping
  if ( (gamemode == commercial)
    && ( finalecount > 50) )
  {
    // go on to the next level
    for (i=0 ; i<MAXPLAYERS ; i++)
      if (players[i].cmd.buttons)
        break;

    if (i < MAXPLAYERS)
    { 
      if (gamemap == 30)
        F_StartCast ();
      else
        gameaction = ga_worlddone;
    }
  }

  // advance animation
  finalecount++;

  if (finalestage == 2)
  {
    F_CastTicker ();
    return;
  }

  if ( gamemode == commercial)
    return;

  if (!finalestage && finalecount>CGDstrlen (finaletext)*TEXTSPEED + TEXTWAIT)
  {
    finalecount = 0;
    finalestage = 1;
    wipegamestate = (gamestate_t)-1;  // force a wipe
  }
}



//
// F_TextWrite
//

#include "hu_stuff.h"
extern const patch_t *hu_font[HU_FONTSIZE];


void F_TextWrite (void)
{
  const byte* src;
  byte* dest;

  int  x,y,w;
  int  count;
  const char* ch;
  int  c;
  int  cx;
  int  cy;

  // erase the entire screen to a tiled background
  src = (const byte*)W_CacheLumpNameConst( finaleflat , PU_CACHE);
  dest = (byte*)screens[0];

  for (y=0 ; y<SCREENHEIGHT ; y++)
  {
    for (x=0 ; x<SCREENWIDTH/64 ; x++)
    {
      memcpy (dest, src+((y&63)<<6), 64);
      dest += 64;
    }
  }

  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

  // draw some of the text onto the screen
  cx = 10;
  cy = 10;
  ch = finaletext;

  count = (finalecount - 10)/TEXTSPEED;
  if (count < 0)
    count = 0;
  for ( ; count ; count-- )
  {
    c = *ch++;
    if (!c)
      break;
    if (c == '\n')
    {
      cx = 10;
      cy += 11;
      continue;
    }

    c = toupper_int(c) - HU_FONTSTART;

    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }

    w = SHORT (hu_font[c]->width);
    if (cx+w > SCREENWIDTH)
      break;
    V_DrawPatch(cx, cy, 0, hu_font[c]);
    cx+=w;
  }

}

//
// Final DOOM 2 animation
// Casting by id Software.
//   in order of appearance
//
typedef struct
{
  const char  *name;
  mobjtype_t type;
} castinfo_t;

const castinfo_t castorder[] = {
  {CC_ZOMBIE, MT_POSSESSED},
  {CC_SHOTGUN, MT_SHOTGUY},
  {CC_HEAVY, MT_CHAINGUY},
  {CC_IMP, MT_TROOP},
  {CC_DEMON, MT_SERGEANT},
  {CC_LOST, MT_SKULL},
  {CC_CACO, MT_HEAD},
  {CC_HELL, MT_KNIGHT},
  {CC_BARON, MT_BRUISER},
  {CC_ARACH, MT_BABY},
  {CC_PAIN, MT_PAIN},
  {CC_REVEN, MT_UNDEAD},
  {CC_MANCU, MT_FATSO},
  {CC_ARCH, MT_VILE},
  {CC_SPIDER, MT_SPIDER},
  {CC_CYBER, MT_CYBORG},
  {CC_HERO, MT_PLAYER},

  {NULL,(mobjtype_t)0}
};

int  castnum;
int  casttics;
state_t* caststate;
boolean  castdeath;
int  castframes;
int  castonmelee;
boolean  castattacking;


//
// F_StartCast
//
extern gamestate_t     wipegamestate;


void F_StartCast (void)
{
  wipegamestate = (gamestate_t)-1;  // force a screen wipe
  castnum = 0;
  caststate = (state_t*)&states[mobjinfo[castorder[castnum].type].seestate];
  casttics = caststate->tics;
  castdeath = false;
  finalestage = 2; 
  castframes = 0;
  castonmelee = 0;
  castattacking = false;
}


//
// F_CastTicker
//
void F_CastTicker (void)
{
  int  st;

  if (--casttics > 0)
    return;   // not time to change state yet

  if (caststate->tics == -1 || caststate->nextstate == S_NULL)
  {
    // switch from deathstate to next monster
    castnum++;
    castdeath = false;
    if (castorder[castnum].name == NULL)
      castnum = 0;
    caststate = (state_t*)&states[mobjinfo[castorder[castnum].type].seestate];
    castframes = 0;
  }
  else
  {
    // just advance to next state in animation
    if (caststate == &states[S_PLAY_ATK1])
      goto stopattack; // Oh, gross hack!
    st = caststate->nextstate;
    caststate = (state_t*)&states[st];
    castframes++;
  }

  if (castframes == 12)
  {
    // go into attack frame
    castattacking = true;
    if (castonmelee)
      caststate=(state_t*)&states[mobjinfo[castorder[castnum].type].meleestate];
    else
      caststate=(state_t*)&states[mobjinfo[castorder[castnum].type].missilestate];

    castonmelee ^= 1;

    if (caststate == &states[S_NULL])
    {
      if (castonmelee)
        caststate=(state_t*)&states[mobjinfo[castorder[castnum].type].meleestate];
      else
        caststate=(state_t*)&states[mobjinfo[castorder[castnum].type].missilestate];
    }
  }

  if (castattacking)
  {
    if (castframes == 24
      || caststate == &states[mobjinfo[castorder[castnum].type].seestate] )
    {
stopattack:
      castattacking = false;
      castframes = 0;
      caststate = (state_t*)&states[mobjinfo[castorder[castnum].type].seestate];
    }
  }

  casttics = caststate->tics;

  if (casttics == -1)
    casttics = 15;
}



//
// F_CastResponder
//

boolean F_CastResponder (event_t* ev)
{
  if (ev->type != ev_keydown)
    return false;

  if (castdeath)
    return true;   // already in dying frames

  // go into death frame
  castdeath = true;
  caststate = (state_t*)&states[mobjinfo[castorder[castnum].type].deathstate];
  casttics = caststate->tics;
  castframes = 0;
  castattacking = false;

  return true;
}


void F_CastPrint (const char* text)
{
  const char* ch;
  int  c;
  int  cx;
  int  w;
  int  width;

  // find width
  ch = text;
  width = 0;

  while (ch)
  {
    c = *ch++;
    if (!c)
      break;
    c = toupper_int(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      width += 4;
      continue;
    }

    w = SHORT (hu_font[c]->width);
    width += w;
  }

  // draw it
  cx = 160-width/2;
  ch = text;
  while (ch)
  {
    c = *ch++;
    if (!c)
      break;
    c = toupper_int(c) - HU_FONTSTART;
    if (c < 0 || c> HU_FONTSIZE)
    {
      cx += 4;
      continue;
    }

    w = SHORT (hu_font[c]->width);
    V_DrawPatch(cx, 180, 0, hu_font[c]);
    cx+=w;
  }

}


//
// F_CastDrawer
//
void V_DrawPatchFlipped (int x, int y, int scrn, const patch_t *patch);

void F_CastDrawer (void)
{
  spritedef_t* sprdef;
  spriteframe_t* sprframe;
  int   lump;
  boolean  flip;
  const patch_t*  patch;

  // erase the entire screen to a background
  V_DrawPatch (0,0,0,(const patch_t*) W_CacheLumpNameConst("BOSSBACK", PU_CACHE));

  F_CastPrint (castorder[castnum].name);

  // draw the current frame in the middle of the screen
  sprdef = &sprites[caststate->sprite];
  sprframe = &sprdef->spriteframes[ caststate->frame & FF_FRAMEMASK];
  lump = sprframe->lump[0];
  flip = (boolean)sprframe->flip[0];

  patch = (const patch_t*)W_CacheLumpNumConst ((lump+firstspritelump), PU_CACHE);
  if (flip)
    V_DrawPatchFlipped (160,170,0,patch);
  else
    V_DrawPatch (160,170,0,patch);
}


//
// F_DrawPatchCol
//
void F_DrawPatchCol( int  x,const patch_t* patch, int  col )
{
  column_t* column;
  byte* source;
  byte* dest;
  byte* desttop;
  int  count;

  column = (column_t *)((byte *)patch + INTFromBytes((const byte*)&(patch->columnofsLE[col])));
  desttop = screens[0]+x;

  // step through the posts in a column
  while (column->topdelta != 0xff )
  {
    source = (byte *)column + 3;
    dest = desttop + column->topdelta*SCREENWIDTH;
    count = column->length;

    while (count--)
    {
      *dest = *source++;
      dest += SCREENWIDTH;
    }
    column = (column_t *)(  (byte *)column + column->length + 4 );
  }
}


//
// F_BunnyScroll
//
void F_BunnyScroll (void)
{
  int  scrolled;
  int  x;
  const patch_t* p1;
  const patch_t* p2;
  char name[10];
  int  stage;
  static int laststage;

  p1 = W_CacheLumpNamePatch("PFUB2", PU_LEVEL);
  p2 = W_CacheLumpNamePatch("PFUB1", PU_LEVEL);

  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);

  scrolled = 320 - (finalecount-230)/2;
  if (scrolled > 320)
    scrolled = 320;
  if (scrolled < 0)
    scrolled = 0;

  for ( x=0 ; x<SCREENWIDTH ; x++)
  {
    if (x+scrolled < 320)
      F_DrawPatchCol (x, p1, x+scrolled);
    else
      F_DrawPatchCol (x, p2, x+scrolled - 320);  
  }

  if (finalecount < 1130)
    return;
  if (finalecount < 1180)
  {
    V_DrawPatch ((SCREENWIDTH-13*8)/2,(SCREENHEIGHT-8*8)/2,0, (const patch_t*)W_CacheLumpNameConst("END0",PU_CACHE));
    laststage = 0;
    return;
  }

  stage = (finalecount-1180) / 5;
  if (stage > 6)
    stage = 6;
  if (stage > laststage)
  {
    laststage = stage;
  }

  //sprintf (name,"END%i",stage);
  CGDAppendNum09("END",stage,name);
  V_DrawPatch ((SCREENWIDTH-13*8)/2, (SCREENHEIGHT-8*8)/2,0, (const patch_t*)W_CacheLumpNameConst(name,PU_CACHE));
}


//
// F_Drawer
//
void F_Drawer (void)
{
  if (finalestage == 2)
  {
    F_CastDrawer ();
    return;
  }

  if (!finalestage)
    F_TextWrite ();
  else
  {
    switch (gameepisode)
    {
    case 1:
      if ( gamemode == retail )
        V_DrawPatch (0,0,0,(const patch_t*)W_CacheLumpNameConst("CREDIT",PU_CACHE));
      else
        V_DrawPatch (0,0,0,(const patch_t*)W_CacheLumpNameConst("HELP2",PU_CACHE));
      break;
    case 2:
      V_DrawPatch(0,0,0,(const patch_t*)W_CacheLumpNameConst("VICTORY2",PU_CACHE));
      break;
    case 3:
      F_BunnyScroll ();
      break;
    case 4:
      V_DrawPatch (0,0,0,(const patch_t*)W_CacheLumpNameConst("ENDPIC",PU_CACHE));
      break;
    }
  }
}

