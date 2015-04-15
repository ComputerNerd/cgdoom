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
// Intermission screens.
//
//-----------------------------------------------------------------------------

//static const char 

#include "os.h"

#include "z_zone.h"

#include "m_random.h"
#include "m_swap.h"

#include "i_system.h"

#include "w_wad.h"

#include "g_game.h"

#include "r_local.h"
//#include "s_sound.h"

#include "doomstat.h"

// Data.
//#include "sounds.h"

// Needs access to LFB.
#include "v_video.h"

#include "wi_stuff.h"

//
// Data needed to add patches to full screen intermission pics.
// Patches are statistics messages, and animations.
// Loads of by-pixel layout and placement, offsets etc.
//


//
// Different vetween registered DOOM (1994) and
//  Ultimate DOOM - Final edition (retail, 1995?).
// This is supposedly ignored for commercial
//  release (aka DOOM II), which had 34 maps
//  in one episode. So there.
#define NUMEPISODES 4
#define NUMMAPS  9


// in tics
//U #define PAUSELEN  (TICRATE*2) 
//U #define SCORESTEP  100
//U #define ANIMPERIOD  32
// pixel distance from "(YOU)" to "PLAYER N"
//U #define STARDIST  10 
//U #define WK 1


// GLOBAL LOCATIONS
#define WI_TITLEY  2
#define WI_SPACINGY      33

// SINGPLE-PLAYER STUFF
#define SP_STATSX  50
#define SP_STATSY  50

#define SP_TIMEX  16
#define SP_TIMEY  (SCREENHEIGHT-32)


// NET GAME STUFF
//#define NG_STATSY  50
//#define NG_STATSX  (32 + SHORT(star->width)/2 + 32*!dofrags)

//#define NG_SPACINGX      64


// DEATHMATCH STUFF
//#define DM_MATRIXX  42
//#define DM_MATRIXY  68

//#define DM_SPACINGX  40

//#define DM_TOTALSX  269

//#define DM_KILLERSX  10
//#define DM_KILLERSY  100
//#define DM_VICTIMSX      5
//#define DM_VICTIMSY  50




typedef enum
{
  ANIM_ALWAYS,
  ANIM_RANDOM,
  ANIM_LEVEL

} animenum_t;

typedef struct
{
  short  x;
  short  y;

} point_t;


//
// Animation.
// There is another wi_stuff_anim_t used in p_spec.
//
typedef struct
{
  animenum_t type;

  // period in tics between animations
  int  period;

  // number of animation frames
  int  nanims;

  // location of animation
  point_t loc;

  // ALWAYS: n/a,
  // RANDOM: period deviation (<256),
  // LEVEL: level
  int  data1;

  // ALWAYS: n/a,
  // RANDOM: random base period,
  // LEVEL: n/a
  int  data2; 

  // actual graphics for frames of animations
  const patch_t* p[3]; 

  // following must be initialized to zero before use!

  // next value of bcnt (used in conjunction with period)
  int  nexttic;

  // last drawn animation frame
  int  lastdrawn;

  // next frame number to animate
  int  ctr;

  // used by RANDOM and LEVEL when animating
  int  state;  

} wi_stuff_anim_t;


static const point_t lnodes[NUMEPISODES][NUMMAPS] =
{
  // Episode 0 World Map
  {
    { 185, 164 }, // location of level 0 (CJ)
    { 148, 143 }, // location of level 1 (CJ)
    { 69, 122 }, // location of level 2 (CJ)
    { 209, 102 }, // location of level 3 (CJ)
    { 116, 89 }, // location of level 4 (CJ)
    { 166, 55 }, // location of level 5 (CJ)
    { 71, 56 }, // location of level 6 (CJ)
    { 135, 29 }, // location of level 7 (CJ)
    { 71, 24 } // location of level 8 (CJ)
  },

    // Episode 1 World Map should go here
  {
    { 254, 25 }, // location of level 0 (CJ)
    { 97, 50 }, // location of level 1 (CJ)
    { 188, 64 }, // location of level 2 (CJ)
    { 128, 78 }, // location of level 3 (CJ)
    { 214, 92 }, // location of level 4 (CJ)
    { 133, 130 }, // location of level 5 (CJ)
    { 208, 136 }, // location of level 6 (CJ)
    { 148, 140 }, // location of level 7 (CJ)
    { 235, 158 } // location of level 8 (CJ)
  },

    // Episode 2 World Map should go here
  {
    { 156, 168 }, // location of level 0 (CJ)
    { 48, 154 }, // location of level 1 (CJ)
    { 174, 95 }, // location of level 2 (CJ)
    { 265, 75 }, // location of level 3 (CJ)
    { 130, 48 }, // location of level 4 (CJ)
    { 279, 23 }, // location of level 5 (CJ)
    { 198, 48 }, // location of level 6 (CJ)
    { 140, 25 }, // location of level 7 (CJ)
    { 281, 136 } // location of level 8 (CJ)
  }

};


//
// Animation locations for episode 0 (1).
// Using patches saves a lot of space,
//  as they replace 320x200 full screen frames.
//
static wi_stuff_anim_t epsd0animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 224, 104 }, 0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 184, 160 }, 0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 112, 136 }, 0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 72, 112 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 88, 96 },   0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 48 },   0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 192, 40 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 136, 16 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 80, 16 },   0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 64, 24 },   0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 }
};

static wi_stuff_anim_t epsd1animinfo[] =
{
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 1, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 2, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 3, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 4, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 5, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 6, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 7, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 3, { 192, 144 }, 8, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_LEVEL, TICRATE/3, 1, { 128, 136 }, 8, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 }
};

static wi_stuff_anim_t epsd2animinfo[] =
{
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 168 }, 0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 40, 136 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 160, 96 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 104, 80 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/3, 3, { 120, 32 },  0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 },
  { ANIM_ALWAYS, TICRATE/4, 3, { 40, 0 },    0, 0, {NULL,NULL,NULL}, 0, 0, 0, 0 }
};

static int NUMANIMS[NUMEPISODES] =
{
  sizeof(epsd0animinfo)/sizeof(wi_stuff_anim_t),
    sizeof(epsd1animinfo)/sizeof(wi_stuff_anim_t),
    sizeof(epsd2animinfo)/sizeof(wi_stuff_anim_t)
};

static wi_stuff_anim_t *wi_stuff_anims[NUMEPISODES] =
{
  epsd0animinfo,
    epsd1animinfo,
    epsd2animinfo
};


//
// GENERAL DATA
//

//
// Locally used stuff.
//
#define FB 0


// States for single-player
#define SP_KILLS  0
#define SP_ITEMS  2
#define SP_SECRET  4
#define SP_FRAGS  6 
#define SP_TIME   8 
#define SP_PAR   ST_TIME

#define SP_PAUSE  1

// in seconds
#define SHOWNEXTLOCDELAY 4
//#define SHOWLASTLOCDELAY SHOWNEXTLOCDELAY


// used to accelerate or skip a stage
static int  acceleratestage;

// wbs->pnum
static int  me;

// specifies current state
static stateenum_t state;

// contains information passed into intermission
static wbstartstruct_t* wbs;

static wbplayerstruct_t* plrs;  // wbs->plyr[]

// used for general timing
static int   cnt;  

// used for timing of background animation
static int   bcnt;

// signals to refresh everything for one frame
static int   firstrefresh; 

static int  cnt_kills[MAXPLAYERS];
static int  cnt_items[MAXPLAYERS];
static int  cnt_secret[MAXPLAYERS];
static int  cnt_time;
static int  cnt_par;
static int  cnt_pause;

// # of commercial levels
static int  NUMCMAPS; 


//
// GRAPHICS
//

// background (map of levels).
static const patch_t*  bg;

// You Are Here graphic
static const patch_t*  yah[2]; 

// splat
static const patch_t*  splat;

// %, : graphics
static const patch_t*  percent;
static const patch_t*  colon;

// 0-9 graphic
static const patch_t*  num[10];

// minus sign
static const patch_t*  wiminus;

// "Finished!" graphics
static const patch_t*  finished;

// "Entering" graphic
static const patch_t*  entering; 

// "secret"
static const patch_t*  sp_secret;

// "Kills", "Scrt", "Items", "Frags"
static const patch_t*  kills;
static const patch_t*  secret;
static const patch_t*  items;
static const patch_t*  frags;

// Time sucks.
static const patch_t*  time;
static const patch_t*  par;
static const patch_t*  sucks;

// "killers", "victims"
static const patch_t*  killers;
static const patch_t*  victims; 

// "Total", your face, your dead face
static const patch_t*  total;
static const patch_t*  star;
static const patch_t*  bstar;

// "red P[1..MAXPLAYERS]"
//static patch_t*  p[MAXPLAYERS];

// "gray P[1..MAXPLAYERS]"
//static patch_t*  bp[MAXPLAYERS];

// Name graphics of each level (centered)
static const patch_t** lnames;

//Prototypes
void WI_unloadData(void);

//
// CODE
//

// slam background
// UNUSED static unsigned char *background=0;


static void WI_slamBackground(void)
{
  memcpy(screens[0], screens[1], SCREENWIDTH * SCREENHEIGHT);
  V_MarkRect (0, 0, SCREENWIDTH, SCREENHEIGHT);
}

// Draws "<Levelname> Finished!"
static void WI_drawLF(void)
{
  int y = WI_TITLEY;

  // draw <LevelName> 
  V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->last]->width))/2,y, FB, lnames[wbs->last]);

  // draw "Finished!"
  y += (5*SHORT(lnames[wbs->last]->height))/4;

  V_DrawPatch((SCREENWIDTH - SHORT(finished->width))/2,y, FB, finished);
}



// Draws "Entering <LevelName>"
static void WI_drawEL(void)
{
  int y = WI_TITLEY;

  // draw "Entering"
  V_DrawPatch((SCREENWIDTH - SHORT(entering->width))/2, y, FB, entering);

  // draw level
  y += (5*SHORT(lnames[wbs->next]->height))/4;

  V_DrawPatch((SCREENWIDTH - SHORT(lnames[wbs->next]->width))/2, y, FB, lnames[wbs->next]);

}

static void WI_drawOnLnode( int n,const patch_t* c[] )
{

  int  i;
  int  left;
  int  top;
  int  right;
  int  bottom;
  boolean fits = false;

  i = 0;
  do
  {
    left = lnodes[wbs->epsd][n].x - SHORT(c[i]->leftoffset);
    top = lnodes[wbs->epsd][n].y - SHORT(c[i]->topoffset);
    right = left + SHORT(c[i]->width);
    bottom = top + SHORT(c[i]->height);

    if (left >= 0 && right < SCREENWIDTH && top >= 0 && bottom < SCREENHEIGHT)
    {
      fits = true;
    }
    else
    {
      i++;
    }
  } while (!fits && i!=2);

  if (fits && i<2)
  {
    V_DrawPatch(lnodes[wbs->epsd][n].x, lnodes[wbs->epsd][n].y, FB, c[i]);
  }
  else
  {
    // DEBUG
    //printf("Could not place patch on level %d", n+1); 
  }
}



static void WI_initAnimatedBack(void)
{
  int  i;
  wi_stuff_anim_t* a;

  if (gamemode == commercial)
    return;

  if (wbs->epsd > 2)
    return;

  for (i=0;i<NUMANIMS[wbs->epsd];i++)
  {
    //a = &wi_stuff_anims[wbs->epsd][i];
    if (wbs->epsd==0)
      a= &epsd0animinfo[i];
    else if (wbs->epsd==1)
      a= &epsd1animinfo[i];
    else
      a= &epsd2animinfo[i];

    // init variables
    a->ctr = -1;

    // specify the next time to draw it
    if (a->type == ANIM_ALWAYS)
      a->nexttic = bcnt + 1 + (M_Random()%a->period);
    else if (a->type == ANIM_RANDOM)
      a->nexttic = bcnt + 1 + a->data2+(M_Random()%a->data1);
    else if (a->type == ANIM_LEVEL)
      a->nexttic = bcnt + 1;
  }

}

static void WI_updateAnimatedBack(void)
{
  int  i;
  wi_stuff_anim_t* a;

  if (gamemode == commercial)
    return;

  if (wbs->epsd > 2)
    return;

  for (i=0;i<NUMANIMS[wbs->epsd];i++)
  {
    //a = &wi_stuff_anims[wbs->epsd][i];
    if (wbs->epsd==0)
      a= &epsd0animinfo[i];
    else if (wbs->epsd==1)
      a= &epsd1animinfo[i];
    else
      a= &epsd2animinfo[i];

    if (bcnt == a->nexttic)
    {
      switch (a->type)
      {
      case ANIM_ALWAYS:
        if (++a->ctr >= a->nanims) a->ctr = 0;
        a->nexttic = bcnt + a->period;
        break;

      case ANIM_RANDOM:
        a->ctr++;
        if (a->ctr == a->nanims)
        {
          a->ctr = -1;
          a->nexttic = bcnt+a->data2+(M_Random()%a->data1);
        }
        else a->nexttic = bcnt + a->period;
        break;

      case ANIM_LEVEL:
        // gawd-awful hack for level wi_stuff_anims
        if (!(state == StatCount && i == 7)
          && wbs->next == a->data1)
        {
          a->ctr++;
          if (a->ctr == a->nanims) a->ctr--;
          a->nexttic = bcnt + a->period;
        }
        break;
      }
    }
  }
}

static void WI_drawAnimatedBack(void)
{

}

//
// Draws a number.
// If digits > 0, then use that many digits minimum,
//  otherwise only use as many as necessary.
// Returns new x position.
//

static int WI_drawNum (int  x,int  y,int  n,int  digits )
{

  int  fontwidth = SHORT(num[0]->width);
  int  neg;
  int  temp;

  if (digits < 0)
  {
    if (!n)
    {
      // make variable-length zeros 1 digit long
      digits = 1;
    }
    else
    {
      // figure out # of digits in #
      digits = 0;
      temp = n;

      while (temp)
      {
        temp /= 10;
        digits++;
      }
    }
  }

  neg = n < 0;
  if (neg)
    n = -n;

  // if non-number, do not draw it
  if (n == 1994)
    return 0;

  // draw the new number
  while (digits--)
  {
    x -= fontwidth;
    V_DrawPatch(x, y, FB, num[ n % 10 ]);
    n /= 10;
  }

  // draw a minus sign if necessary
  if (neg)
    V_DrawPatch(x-=8, y, FB, wiminus);

  return x;

}

static void WI_drawPercent ( int  x,int  y,int  p )
{
  if (p < 0)
    return;

  V_DrawPatch(x, y, FB, percent);
  WI_drawNum(x, y, p, -1);
}



//
// Display level completion time and par,
//  or "sucks" message if overflow.
//
static void WI_drawTime( int  x,  int  y,  int  t )
{

  int  div;
  int  n;

  if (t<0)
    return;

  if (t <= 61*59)
  {
    div = 1;

    do
    {
      n = (t / div) % 60;
      x = WI_drawNum(x, y, n, 2) - SHORT(colon->width);
      div *= 60;

      // draw
      if (div==60 || t / div)
        V_DrawPatch(x, y, FB, colon);

    } while (t / div);
  }
  else
  {
    // "sucks"
    V_DrawPatch(x - SHORT(sucks->width), y, FB, sucks); 
  }
}


static void WI_End(void)
{
  WI_unloadData();
}

static void WI_initNoState(void)
{
  state = NoState;
  acceleratestage = 0;
  cnt = 10;
}

static void WI_updateNoState(void) 
{

  WI_updateAnimatedBack();

  if (!--cnt)
  {
    WI_End();
    G_WorldDone();
  }
}

static boolean  snl_pointeron = false;


static void WI_initShowNextLoc(void)
{
  state = ShowNextLoc;
  acceleratestage = 0;
  cnt = SHOWNEXTLOCDELAY * TICRATE;

  WI_initAnimatedBack();
}

static void WI_updateShowNextLoc(void)
{
  WI_updateAnimatedBack();

  if (!--cnt || acceleratestage)
    WI_initNoState();
  else
    snl_pointeron = (boolean)((cnt & 31) < 20);
}

static void WI_drawShowNextLoc(void)
{

  int  i;
  int  last;

  WI_slamBackground();

  // draw animated background
  WI_drawAnimatedBack(); 

  if ( gamemode != commercial)
  {
    if (wbs->epsd > 2)
    {
      WI_drawEL();
      return;
    }

    last = (wbs->last == 8) ? wbs->next - 1 : wbs->last;

    // draw a splat on taken cities.
    for (i=0 ; i<=last ; i++)
      WI_drawOnLnode(i, &splat);

    // splat the secret level?
    if (wbs->didsecret)
      WI_drawOnLnode(8, &splat);

    // draw flashing ptr
    if (snl_pointeron)
      WI_drawOnLnode(wbs->next, yah); 
  }

  // draws which level you are entering..
  if ( (gamemode != commercial)
    || wbs->next != 30)
    WI_drawEL();  
}

static void WI_drawNoState(void)
{
  snl_pointeron = true;
  WI_drawShowNextLoc();
}


static int sp_state;

static void WI_initStats(void)
{
  state = StatCount;
  acceleratestage = 0;
  sp_state = 1;
  cnt_kills[0] = cnt_items[0] = cnt_secret[0] = -1;
  cnt_time = cnt_par = -1;
  cnt_pause = TICRATE;

  WI_initAnimatedBack();
}

static void WI_updateStats(void)
{

  WI_updateAnimatedBack();

  if (acceleratestage && sp_state != 10)
  {
    acceleratestage = 0;
    cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
    cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
    cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
    cnt_time = plrs[me].stime / TICRATE;
    cnt_par = wbs->partime / TICRATE;
    sp_state = 10;
  }

  if (sp_state == 2)
  {
    cnt_kills[0] += 2;
    if (cnt_kills[0] >= (plrs[me].skills * 100) / wbs->maxkills)
    {
      cnt_kills[0] = (plrs[me].skills * 100) / wbs->maxkills;
      sp_state++;
    }
  }
  else if (sp_state == 4)
  {
    cnt_items[0] += 2;
    if (cnt_items[0] >= (plrs[me].sitems * 100) / wbs->maxitems)
    {
      cnt_items[0] = (plrs[me].sitems * 100) / wbs->maxitems;
      sp_state++;
    }
  }
  else if (sp_state == 6)
  {
    cnt_secret[0] += 2;

    if (cnt_secret[0] >= (plrs[me].ssecret * 100) / wbs->maxsecret)
    {
      cnt_secret[0] = (plrs[me].ssecret * 100) / wbs->maxsecret;
      sp_state++;
    }
  }

  else if (sp_state == 8)
  {
    cnt_time += 3;

    if (cnt_time >= plrs[me].stime / TICRATE)
      cnt_time = plrs[me].stime / TICRATE;

    cnt_par += 3;

    if (cnt_par >= wbs->partime / TICRATE)
    {
      cnt_par = wbs->partime / TICRATE;

      if (cnt_time >= plrs[me].stime / TICRATE)
        sp_state++;
    }
  }
  else if (sp_state == 10)
  {
    if (acceleratestage)
    {
      if (gamemode == commercial)
        WI_initNoState();
      else
        WI_initShowNextLoc();
    }
  }
  else if (sp_state & 1)
  {
    if (!--cnt_pause)
    {
      sp_state++;
      cnt_pause = TICRATE;
    }
  }
}

static void WI_drawStats(void)
{
  // line height
  int lh; 

  lh = (3*SHORT(num[0]->height))/2;

  WI_slamBackground();

  // draw animated background
  WI_drawAnimatedBack();

  WI_drawLF();

  V_DrawPatch(SP_STATSX, SP_STATSY, FB, kills);
  WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY, cnt_kills[0]);

  V_DrawPatch(SP_STATSX, SP_STATSY+lh, FB, items);
  WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+lh, cnt_items[0]);

  V_DrawPatch(SP_STATSX, SP_STATSY+2*lh, FB, sp_secret);
  WI_drawPercent(SCREENWIDTH - SP_STATSX, SP_STATSY+2*lh, cnt_secret[0]);

  V_DrawPatch(SP_TIMEX, SP_TIMEY, FB, time);
  WI_drawTime(SCREENWIDTH/2 - SP_TIMEX, SP_TIMEY, cnt_time);

  if (wbs->epsd < 3)
  {
    V_DrawPatch(SCREENWIDTH/2 + SP_TIMEX, SP_TIMEY, FB, par);
    WI_drawTime(SCREENWIDTH - SP_TIMEX, SP_TIMEY, cnt_par);
  }

}

static void WI_checkForAccelerate(void)
{
  int   i;
  player_t  *player;

  // check for button presses to skip delays
  for (i=0, player = players ; i<MAXPLAYERS ; i++, player++)
  {
    if (playeringame[i])
    {
      if (player->cmd.buttons & BT_ATTACK)
      {
        if (!player->attackdown)
          acceleratestage = 1;
        player->attackdown = true;
      }
      else
        player->attackdown = false;
      if (player->cmd.buttons & BT_USE)
      {
        if (!player->usedown)
          acceleratestage = 1;
        player->usedown = true;
      }
      else
        player->usedown = false;
    }
  }
}



// Updates stuff each tick
void WI_Ticker(void)
{
  // counter for general background animation
  bcnt++;  
  WI_checkForAccelerate();

  switch (state)
  {
  case StatCount:
    WI_updateStats();
    break;

  case ShowNextLoc:
    WI_updateShowNextLoc();
    break;

  case NoState:
    WI_updateNoState();
    break;
  }

}

static void WI_loadData(void)
{
  int  i;
  int  j;
  char name[9];
  wi_stuff_anim_t* a;

  if (gamemode == commercial)
    CGDstrcpy(name, "INTERPIC");
  else 
  {
    //sprintf(name, "WIMAP%d", wbs->epsd);
    CGDAppendNum09("WIMAP",wbs->epsd,name);
  }

  if ( gamemode == retail )
  {
    if (wbs->epsd == 3)
      CGDstrcpy(name,"INTERPIC");
  }

  // background
  bg = W_CacheLumpNamePatch(name, PU_CACHE);
  V_DrawPatch(0, 0, 1, bg);

  // UNUSED unsigned char *pic = screens[1];
  // if (gamemode == commercial)
  // {
  // darken the background image
  // while (pic != screens[1] + SCREENHEIGHT*SCREENWIDTH)
  // {
  //   *pic = colormaps[256*25 + *pic];
  //   pic++;
  // }
  //}

  if (gamemode == commercial)
  {
    NUMCMAPS = 32;        
    lnames = (const patch_t **) Z_Malloc(sizeof(patch_t*) * NUMCMAPS,PU_STATIC, 0);
    for (i=0 ; i<NUMCMAPS ; i++)
    {        
      //   sprintf(name, "CWILV%2.2d", i);
      //sprintf(name, "CWILV%02d", i); // CX port
      CGDAppendNum0_999("CWILV",i,2,name);
      lnames[i] = W_CacheLumpNamePatch(name, PU_STATIC);
    }
  }
  else
  {
    lnames = (const patch_t **) Z_Malloc(sizeof(patch_t*) * NUMMAPS, PU_STATIC, 0);
    for (i=0 ; i<NUMMAPS ; i++)
    {
      //sprintf(name, "WILV%d%d", wbs->epsd, i);
      CGDAppendNum0_999("WILV",wbs->epsd*10+i,2,name);
      lnames[i] = W_CacheLumpNamePatch(name, PU_STATIC);
    }

    // you are here
    yah[0] = W_CacheLumpNamePatch("WIURH0", PU_STATIC);

    // you are here (alt.)
    yah[1] = W_CacheLumpNamePatch("WIURH1", PU_STATIC);

    // splat
    splat = W_CacheLumpNamePatch("WISPLAT", PU_STATIC);

    if (wbs->epsd < 3)
    {
      for (j=0;j<NUMANIMS[wbs->epsd];j++)
      {
        if (wbs->epsd==0)
          a= &epsd0animinfo[j];
        else if (wbs->epsd==1)
          a= &epsd1animinfo[j];
        else
          a= &epsd2animinfo[j];
        //a = &wi_stuff_anims[wbs->epsd][j];  NSPIRE NOT LIKES THIS...
        for (i=0;i<a->nanims;i++)
        {
          // MONDO HACK!
          if (wbs->epsd != 1 || j != 8) 
          {
            CGDAppendNum0_999("WIA",wbs->epsd,1,name);
            CGDAppendNum0_999(name,j,2,name);
            CGDAppendNum09(name,0,name);
            CGDAppendNum0_999(name,i,1,name);
            a->p[i] = W_CacheLumpNamePatch(name, PU_STATIC);
          }
          else
          {
            // HACK ALERT!
            a->p[i] = wi_stuff_anims[1][4].p[i]; 
          }
        }
      }
    }
  }

  // More hacks on minus sign.
  wiminus = W_CacheLumpNamePatch("WIMINUS", PU_STATIC); 

  for (i=0;i<10;i++)
  {
    // numbers 0-9
    //sprintf(name, "WINUM%d", i);     
    CGDAppendNum09("WINUM",i,name);
    num[i] = W_CacheLumpNamePatch(name, PU_STATIC);
  }

  // percent sign
  percent = W_CacheLumpNamePatch("WIPCNT", PU_STATIC);

  // "finished"
  finished = W_CacheLumpNamePatch("WIF", PU_STATIC);

  // "entering"
  entering = W_CacheLumpNamePatch("WIENTER", PU_STATIC);

  // "kills"
  kills = W_CacheLumpNamePatch("WIOSTK", PU_STATIC);   

  // "scrt"
  secret = W_CacheLumpNamePatch("WIOSTS", PU_STATIC);

  // "secret"
  sp_secret = W_CacheLumpNamePatch("WISCRT2", PU_STATIC);

  // Yuck. 
  items = W_CacheLumpNamePatch("WIOSTI", PU_STATIC);

  // "frgs"
  frags = W_CacheLumpNamePatch("WIFRGS", PU_STATIC);    

  // ":"
  colon = W_CacheLumpNamePatch("WICOLON", PU_STATIC); 

  // "time"
  time = W_CacheLumpNamePatch("WITIME", PU_STATIC);   

  // "sucks"
  sucks = W_CacheLumpNamePatch("WISUCKS", PU_STATIC);  

  // "par"
  par = W_CacheLumpNamePatch("WIPAR", PU_STATIC);   

  // "killers" (vertical)
  killers = W_CacheLumpNamePatch("WIKILRS", PU_STATIC);

  // "victims" (horiz)
  victims = W_CacheLumpNamePatch("WIVCTMS", PU_STATIC);

  // "total"
  total = W_CacheLumpNamePatch("WIMSTT", PU_STATIC);   

  // your face
  star = W_CacheLumpNamePatch("STFST01", PU_STATIC);

  // dead face
  bstar = W_CacheLumpNamePatch("STFDEAD0", PU_STATIC);

  //for (i=0 ; i<MAXPLAYERS ; i++)
  //{
  // "1,2,3,4"
  //sprintf(name, "STPB%d", 0);      
  //p = W_CacheLumpNamePatch("STPB0", PU_STATIC);

  // "1,2,3,4"
  //sprintf(name, "WIBP%d", 1);     
  //bp = W_CacheLumpNamePatch("WIBP1", PU_STATIC);
  //}

}

void WI_unloadData(void)
{
  int  i;
  int  j;
  wi_stuff_anim_t a;

  Z_ChangeTag(wiminus, PU_CACHE);

  for (i=0 ; i<10 ; i++)
    Z_ChangeTag(num[i], PU_CACHE);

  if (gamemode == commercial)
  {
    for (i=0 ; i<NUMCMAPS ; i++)
      Z_ChangeTag(lnames[i], PU_CACHE);
  }
  else
  {
    Z_ChangeTag(yah[0], PU_CACHE);
    Z_ChangeTag(yah[1], PU_CACHE);

    Z_ChangeTag(splat, PU_CACHE);

    for (i=0 ; i<NUMMAPS ; i++)
      Z_ChangeTag(lnames[i], PU_CACHE);

    if (wbs->epsd < 3)
    {
      for (j=0;j<NUMANIMS[wbs->epsd];j++)
      {
        if (wbs->epsd != 1 || j != 8)
        {
          //for (i=0;i<wi_stuff_anims[wbs->epsd][j].nanims;i++)    NSPIRE DOESN'T SEEM TO LIKE THIS FOR SOME REASON...
          // Z_ChangeTag(wi_stuff_anims[wbs->epsd][j].p[i], PU_CACHE);
          if (wbs->epsd==0)
            a= epsd0animinfo[j];
          else if (wbs->epsd==1)
            a= epsd1animinfo[j];
          else
            a= epsd2animinfo[j];
          for (i=0;i<a.nanims;i++)
            Z_ChangeTag(a.p[i], PU_CACHE);
        }
      }
    }
  }

  Z_Free(lnames);

  Z_ChangeTag(percent, PU_CACHE);
  Z_ChangeTag(colon, PU_CACHE);
  Z_ChangeTag(finished, PU_CACHE);
  Z_ChangeTag(entering, PU_CACHE);
  Z_ChangeTag(kills, PU_CACHE);
  Z_ChangeTag(secret, PU_CACHE);
  Z_ChangeTag(sp_secret, PU_CACHE);
  Z_ChangeTag(items, PU_CACHE);
  Z_ChangeTag(frags, PU_CACHE);
  Z_ChangeTag(time, PU_CACHE);
  Z_ChangeTag(sucks, PU_CACHE);
  Z_ChangeTag(par, PU_CACHE);

  Z_ChangeTag(victims, PU_CACHE);
  Z_ChangeTag(killers, PU_CACHE);
  Z_ChangeTag(total, PU_CACHE);

}

void WI_Drawer (void)
{
  switch (state)
  {
  case StatCount:
    WI_drawStats();
    break;

  case ShowNextLoc:
    WI_drawShowNextLoc();
    break;

  case NoState:
    WI_drawNoState();
    break;
  }
}


static void WI_initVariables(wbstartstruct_t* wbstartstruct)
{

  wbs = wbstartstruct;

#ifdef RANGECHECKING
  if (gamemode != commercial)
  {
    if ( gamemode == retail )
      RNGCHECK(wbs->epsd, 0, 3);
    else
      RNGCHECK(wbs->epsd, 0, 2);
  }
  else
  {
    RNGCHECK(wbs->last, 0, 8);
    RNGCHECK(wbs->next, 0, 8);
  }
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
  RNGCHECK(wbs->pnum, 0, MAXPLAYERS);
#endif

  acceleratestage = 0;
  cnt = bcnt = 0;
  firstrefresh = 1;
  me = wbs->pnum;
  plrs = wbs->plyr;

  if (!wbs->maxkills)
    wbs->maxkills = 1;

  if (!wbs->maxitems)
    wbs->maxitems = 1;

  if (!wbs->maxsecret)
    wbs->maxsecret = 1;

  if ( gamemode != retail )
    if (wbs->epsd > 2)
      wbs->epsd -= 3;
}

void WI_Start(wbstartstruct_t* wbstartstruct)
{
  WI_initVariables(wbstartstruct);
  WI_loadData();

  WI_initStats();
}
