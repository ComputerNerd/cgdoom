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
// DESCRIPTION:  none
//
//-----------------------------------------------------------------------------


//static const char 

#include "os.h"

#include "doomdef.h" 
#include "doomstat.h"

#include "z_zone.h"
#include "f_finale.h"
//#include "m_argv.h"
#include "m_misc.h"
#include "m_menu.h"
#include "m_random.h"
#include "i_system.h"

#include "p_setup.h"
#include "p_saveg.h"
#include "p_tick.h"

#include "d_main.h"

#include "wi_stuff.h"
#include "hu_stuff.h"
#include "st_stuff.h"
#include "am_map.h"

// Needs access to LFB.
#include "v_video.h"

#include "w_wad.h"

#include "p_local.h" 

// Data.
#include "dstrings.h"

// SKY handling - still the wrong place.
#include "r_data.h"
#include "r_sky.h"



#include "g_game.h"


#define SAVEGAMESIZE 0x2c000
#define SAVESTRINGSIZE 24



boolean     G_CheckDemoStatus (void); 
void     G_ReadDemoTiccmd (ticcmd_t* cmd); 
void     G_WriteDemoTiccmd (ticcmd_t* cmd); 
void     G_PlayerReborn (int player); 
void     G_InitNew (skill_t skill, int episode, int map); 

void     G_DoReborn (void); 

void     G_DoLoadLevel (void); 
void     G_DoNewGame (void); 
void     G_DoLoadGame (void); 
void     G_DoCompleted (void); 
void     G_DoVictory (void); 
void     G_DoWorldDone (void); 
void     G_DoSaveGame (void); 


gameaction_t   gameaction; 
gamestate_t    gamestate; 
skill_t     gameskill; 
boolean     respawnmonsters;
int      gameepisode; 
int      gamemap; 

boolean     paused; 
boolean     sendpause;              // send a pause event next tic 
boolean     sendsave;              // send a save event next tic 
boolean     usergame;               // ok to save / end game 

boolean     timingdemo;             // if true, exit with report on completion 
boolean     nodrawers;              // for comparative timing purposes 
boolean     noblit;                 // for comparative timing purposes 
int      starttime;           // for comparative timing purposes    

boolean     viewactive; 

boolean     deathmatch;            // only if started as net death 
const boolean     netgame = 0;                // only true if packets are broadcast 
boolean     playeringame[MAXPLAYERS]; 
player_t    players[MAXPLAYERS]; 

const      int consoleplayer = 0;          // player taking events and displaying 
int      displayplayer;          // view being displayed 
int                gametic; 
int               levelstarttic;          // gametic at level start 
int               totalkills, totalitems, totalsecret;    // for intermission 

//char     demoname[32]; 
const boolean     netdemo = 0; 
byte*     demobuffer;
byte*     demo_p;
byte*     demoend; 
boolean           singledemo;             // quit after playing a demo from cmdline 

const boolean          precache = false;        // if true, load all graphics at start 

wbstartstruct_t   wminfo;                // parms for world map / intermission 

byte*     savebuffer;


// 
// controls (have defaults) 
// 
int               key_right= KEY_RIGHTARROW;
int      key_left= KEY_LEFTARROW;

int      key_up= KEY_UPARROW;
int      key_down= KEY_DOWNARROW; 
int               key_strafeleft= KEY_SRIGHTARROW;
int      key_straferight= KEY_SLEFTARROW; 
int               key_fire= KEY_RCTRL;
int      key_use= 32;
int      key_strafe;
int      key_speed= KEY_RSHIFT; 

#define MAXPLMOVE  (forwardmove[1]) 

#define TURBOTHRESHOLD 0x32

const fixed_t     forwardmove[2] = {0x19, 0x32}; 
const fixed_t     sidemove[2] = {0x18, 0x28}; 
const fixed_t     angleturn[3] = {640, 1280, 320}; // + slow turn 

#define SLOWTURNTICS 6 

#define NUMKEYS   256 

boolean           gamekeydown[NUMKEYS]; 
int               turnheld;    // for accelerative turning 

int      savegameslot; 
char     savedescription[32]; 


#define BODYQUESIZE  32

mobj_t*     bodyque[BODYQUESIZE]; 
int      bodyqueslot; 

void*     statcopy;    // for statistics driver


/* I HOPE THIS ISN'T NEEDED, BECAUSE IT GIVES A STRANGE COMPILER WARNING THAT I HAVE NO IDEA WHAT TO DO WITH
int G_CmdChecksum (ticcmd_t* cmd) 
{ 
int  i;
int  sum = 0; 

for (i=0 ; i< sizeof(*cmd)/4 - 1 ; i++) 
sum += ((int *)cmd)[i]; 

return sum;
} 
*/
//
// G_BuildTiccmd
// Builds a ticcmd from all of the available inputs
// or reads it from the demo buffer. 
// If recording a demo, write it out 
// 
void G_BuildTiccmd (ticcmd_t* cmd) 
{ 
  int   i; 
  //boolean  strafe;
  int   speed;
  int   tspeed; 
  int   forward;
  int   side;

  ticcmd_t* base;

  base = I_BaseTiccmd ();  // empty, or external driver
  memcpy (cmd,base,sizeof(*cmd)); 

  //cmd->consistancy = consistancy[consoleplayer][maketic%BACKUPTICS];

  speed = gamekeydown[key_speed];

  forward = side = 0;

  /* NO IDEA WHAT THIS DOES, LET'S KEEP IT COMMENTED AND HOPE IT DOESN'T BREAK ANYTHING
  // use two stage accelerative turning on the keyboard
  if (gamekeydown[key_right] || gamekeydown[key_left]) 
  turnheld += ticdup; 
  else 
  turnheld = 0; 
  */
  /*if (turnheld < SLOWTURNTICS) 
  tspeed = 2;             // slow turn 
  else */
  tspeed = speed;

  if (gamekeydown[key_right])
    cmd->angleturn -= angleturn[tspeed];
  if (gamekeydown[key_left])
    cmd->angleturn += angleturn[tspeed];

  if (gamekeydown[key_up])
    forward= +forwardmove[speed];

  if (gamekeydown[key_down])
    forward= -forwardmove[speed];

  if (gamekeydown[key_strafeleft])
    side= +sidemove[speed];
  if (gamekeydown[key_straferight])
    side= -sidemove[speed];

  // buttons
  //cmd->chatchar = HU_dequeueChatChar(); 

  if (gamekeydown[key_fire])
    cmd->buttons |= BT_ATTACK;

  if (gamekeydown[key_use] ) 
    cmd->buttons |= BT_USE;

  // chainsaw overrides 
  for (i=0 ; i<NUMWEAPONS-1 ; i++)
    if (gamekeydown['1'+i]) 
    { 
      cmd->buttons |= BT_CHANGE; 
      cmd->buttons |= i<<BT_WEAPONSHIFT; 
      break; 
    }

    if (forward > MAXPLMOVE) 
    {
      forward = MAXPLMOVE; 
    }
    else if (forward < -MAXPLMOVE) 
    {
      forward = -MAXPLMOVE;
    }

    if (side > MAXPLMOVE) 
    {
      side = MAXPLMOVE; 
    }
    else if (side < -MAXPLMOVE) 
    {
      side = -MAXPLMOVE; 
    }

    cmd->forwardmove= forward;
    cmd->sidemove= side;

    // special buttons
    if (sendpause) 
    { 
      sendpause = false; 
      cmd->buttons = BT_SPECIAL | BTS_PAUSE; 
    } 

    if (sendsave) 
    { 
      sendsave = false; 
      cmd->buttons = (byte)(BT_SPECIAL | BTS_SAVEGAME | (savegameslot<<BTS_SAVESHIFT)); 
    } 
} 


//
// G_DoLoadLevel 
//
extern  gamestate_t     wipegamestate; 

void G_DoLoadLevel (void) 
{ 
  //int             i; 

  // Set the sky map.
  // First thing, we have a dummy sky texture name,
  //  a flat. The data is in the WAD only because
  //  we look for an actual index, instead of simply
  //  setting one.
  skyflatnum = R_FlatNumForName ( SKYFLATNAME );

  // DOOM determines the sky texture to be used
  // depending on the current episode, and the game version.
  if ( (gamemode == commercial) || ( gamemode == pack_tnt ) || ( gamemode == pack_plut ) )
  {
    skytexture = R_TextureNumForName ("SKY3");
    if (gamemap < 12)
      skytexture = R_TextureNumForName ("SKY1");
    else
      if (gamemap < 21)
        skytexture = R_TextureNumForName ("SKY2");
  }

  levelstarttic = gametic;        // for time calculation

  if (wipegamestate == GS_LEVEL) 
    wipegamestate = (gamestate_t)-1;             // force a wipe 

  gamestate = GS_LEVEL;

  /*for (i=0 ; i<MAXPLAYERS ; i++) 
  { 
  if (playeringame[i] && players[i].playerstate == PST_DEAD) 
  players[i].playerstate = PST_REBORN; 
  memset (players[i].frags,0,sizeof(players[i].frags)); 
  } */
  memset (players[0].frags,0,sizeof(players[0].frags)); 

  P_SetupLevel (gameepisode, gamemap, 0);    
  displayplayer = consoleplayer;  // view the guy you are playing    
  starttime = I_GetTime (); 
  gameaction = ga_nothing; 
  //Z_CheckHeap ();

  //printf(" \n ...heap OK \n");

  // clear cmd building stuff
  memset (gamekeydown, 0, sizeof(gamekeydown)); 
  sendpause = sendsave = paused = false; 
} 


//
// G_Responder  
// Get info needed to make ticcmd_ts for the players.
// 
boolean G_Responder (event_t* ev) 
{
  /*// allow spy mode changes even during the demo
  if (gamestate == GS_LEVEL && ev->type == ev_keydown && ev->data1 == KEY_F12 && (singledemo || !deathmatch) )
  {
  // spy mode 
  do 
  { 
  displayplayer++; 
  if (displayplayer == MAXPLAYERS) 
  displayplayer = 0; 
  } while (!playeringame[displayplayer] && displayplayer != consoleplayer); 
  return true; 
  }*/

  // any other key pops up menu if in demos
  if (gameaction == ga_nothing && !singledemo && (gamestate == GS_DEMOSCREEN) ) 
  { 
    if (ev->type == ev_keydown ) 
    { 
      M_StartControlPanel (); 
      return true; 
    }
    return false; 
  } 

  //DEBUG DEBUG DEBUG
  //ONE of those three keeps crashing when there's an event, can't be bothered to find which one
  if (gamestate == GS_LEVEL) 
  { 
    if (HU_Responder (ev)) 
      return true; // chat ate the event 
    //if (ST_Responder (ev)) 
    // return true; // status window ate it 
    if (AM_Responder (ev)) 
      return true; // automap ate it
  } 

  if (gamestate == GS_FINALE) 
  { 
    if (F_Responder (ev)) 
      return true; // finale ate the event 
  } 

  switch (ev->type) 
  { 
  case ev_keydown: 
    if (ev->data1 == KEY_PAUSE) 
    { 
      sendpause = true; 
      return true; 
    } 
    if (ev->data1 <NUMKEYS)
    {
      gamekeydown[ev->data1] = true;
    }
    return true;    // eat key down events 

  case ev_keyup: 
    if (ev->data1 <NUMKEYS) 
      gamekeydown[ev->data1] = false; 
    return false;   // always let key up events filter down

  default: 
    break; 
  } 

  return false; 
} 



//
// G_Ticker
// Make ticcmd_ts for the players.
//
void G_Ticker (void) 
{ 
  //int   i;
  ticcmd_t* cmd;

  // do player reborns if needed
  //for (i=0 ; i<MAXPLAYERS ; i++) 
  if (playeringame[0] && players[0].playerstate == PST_REBORN) 
    G_DoReborn ();

  // do things to change the game state
  while (gameaction != ga_nothing) 
  { 
    switch (gameaction) 
    { 
    case ga_loadlevel: 
      G_DoLoadLevel (); 
      break; 
    case ga_newgame: 
      G_DoNewGame (); 
      break; 
    case ga_loadgame: 
      G_DoLoadGame (); 
      break; 
    case ga_savegame: 
      G_DoSaveGame (); 
      break; 
    case ga_completed: 
      G_DoCompleted (); 
      break; 
    case ga_victory: 
      F_StartFinale (); 
      break; 
    case ga_worlddone: 
      G_DoWorldDone (); 
      break; 
    case ga_nothing: 
      break; 
    } 
  }

  //I_Error("loopdaloop2"); //fine as far as this...

  // get commands
  // check for special buttons
  if (playeringame[0]) 
  { 
    cmd = &players[0].cmd;

    G_BuildTiccmd (cmd);

    if (players[0].cmd.buttons & BT_SPECIAL) 
    { 
      switch (players[0].cmd.buttons & BT_SPECIALMASK) 
      { 
      case BTS_PAUSE: 
        paused ^= 1; 
        break; 

      case BTS_SAVEGAME: 
        if (!savedescription[0]) 
          CGDstrcpy(savedescription, "NET GAME"); 

        savegameslot =  (players[0].cmd.buttons & BTS_SAVEMASK)>>BTS_SAVESHIFT; 
        gameaction = ga_savegame;
        break; 
      } 
    } 
  }

  //I_Error("loopdaloop3"); 

  // do main actions
  switch (gamestate) 
  { 
  case GS_LEVEL: 
    P_Ticker ();
    ST_Ticker ();
    AM_Ticker ();
    HU_Ticker ();
    break; 

  case GS_INTERMISSION: 
    WI_Ticker (); 
    break; 

  case GS_FINALE: 
    F_Ticker (); 
    break; 

  case GS_DEMOSCREEN: 
    D_PageTicker (); 
    break; 
  }        
}


//
// PLAYER STRUCTURE FUNCTIONS
// also see P_SpawnPlayer in P_Things
//

//
// G_InitPlayer 
// Called at the start.
// Called by the game initialization functions.
//
void G_InitPlayer (int player) 
{ 
  player_t*  p; 

  // set up the saved info         
  p = &players[player]; 

  // clear everything else to defaults 
  G_PlayerReborn (player); 
} 



//
// G_PlayerFinishLevel
// Can when a player completes a level.
//
void G_PlayerFinishLevel (int player) 
{ 
  player_t* p; 

  p = &players[player]; 

  memset (p->powers, 0, sizeof (p->powers)); 
  memset (p->cards, 0, sizeof (p->cards)); 

  p->mo->flags &= ~MF_SHADOW;  // cancel invisibility 
  p->extralight = 0;   // cancel gun flashes 
  p->fixedcolormap = 0;  // cancel ir gogles 
  p->damagecount = 0;   // no palette changes 
  p->bonuscount = 0; 
} 


//
// G_PlayerReborn
// Called after a player dies 
// almost everything is cleared and initialized 
//
void G_PlayerReborn (int player) 
{ 
  player_t* p; 
  int  i; 
  int  frags[MAXPLAYERS]; 
  int  killcount;
  int  itemcount;
  int  secretcount; 

  memcpy (frags,players[player].frags,sizeof(frags)); 
  killcount = players[player].killcount; 
  itemcount = players[player].itemcount; 
  secretcount = players[player].secretcount; 

  p = &players[player]; 
  memset (p, 0, sizeof(*p)); 

  memcpy (players[player].frags, frags, sizeof(players[player].frags)); 
  players[player].killcount = killcount; 
  players[player].itemcount = itemcount; 
  players[player].secretcount = secretcount; 

  p->usedown = p->attackdown = true; // don't do anything immediately 
  p->playerstate = PST_LIVE;       
  p->health = MAXHEALTH; 
  p->readyweapon = p->pendingweapon = wp_pistol; 
  p->weaponowned[wp_fist] = true; 
  p->weaponowned[wp_pistol] = true; 
  p->ammo[am_clip] = 50; 

  for (i=0 ; i<NUMAMMO ; i++) 
    p->maxammo[i] = maxammo[i]; 
}

//
// G_CheckSpot  
// Returns false if the player cannot be respawned
// at the given mapthing_t spot  
// because something is occupying it 
//
void P_SpawnPlayer (mapthing_t* mthing); 

boolean G_CheckSpot ( int playernum, mapthing_t* mthing ) 
{ 
  fixed_t  x;
  fixed_t  y; 
  subsector_t* ss; 
  unsigned  an; 
  mobj_t*  mo; 
  int   i;

  if (!players[playernum].mo)
  {
    // first spawn of level, before corpses
    for (i=0 ; i<playernum ; i++)
      if (players[i].mo->x == mthing->x << FRACBITS && players[i].mo->y == mthing->y << FRACBITS)
        return false; 
    return true;
  }

  x = mthing->x << FRACBITS; 
  y = mthing->y << FRACBITS; 

  if (!P_CheckPosition (players[playernum].mo, x, y) ) 
    return false; 

  // flush an old corpse if needed 
  if (bodyqueslot >= BODYQUESIZE) 
    P_RemoveMobj (bodyque[bodyqueslot%BODYQUESIZE]); 
  bodyque[bodyqueslot%BODYQUESIZE] = players[playernum].mo; 
  bodyqueslot++; 

  // spawn a teleport fog 
  ss = R_PointInSubsector (x,y); 
  an = ( ANG45 * (mthing->angle/45) ) >> ANGLETOFINESHIFT; 

  mo = P_SpawnMobj (x+20*finecosine(an), y+20*finesine[an], ss->sector->floorheight, MT_TFOG); 

  return true; 
} 


//
// G_DeathMatchSpawnPlayer 
// Spawns a player at one of the random death match spots 
// called at level load and each death 
//
/*void G_DeathMatchSpawnPlayer (int playernum) 
{ 
int             i,j; 
int    selections; 

selections = deathmatch_p - deathmatchstarts; 
if (selections < 4) 
I_Error ("Only %i deathmatch spots, 4 required", selections); 

for (j=0 ; j<20 ; j++) 
{ 
i = P_Random() % selections; 
if (G_CheckSpot (playernum, &deathmatchstarts[i]) ) 
{ 
deathmatchstarts[i].type = playernum+1; 
P_SpawnPlayer (&deathmatchstarts[i]); 
return; 
} 
} 

// no good spot, so the player will probably get stuck 
P_SpawnPlayer (&playerstarts[playernum]); 
} 
*/

//
// G_DoReborn 
// 
void G_DoReborn (void) 
{ 
  // reload the level from scratch
  gameaction = ga_loadlevel;
}

// DOOM Par Times
const int pars[4][10] = 
{ 
  {0}, 
  {0,30,75,120,90,165,180,180,30,165}, 
  {0,90,90,90,120,90,360,240,30,170}, 
  {0,90,45,90,150,90,90,165,30,135} 
}; 

// DOOM II Par Times
const int cpars[32] =
{
  30,90,120,120,90,150,120,120,270,90, //  1-10
    210,150,150,150,210,150,420,150,210,150, // 11-20
    240,150,180,150,150,300,330,420,300,180, // 21-30
    120,30     // 31-32
};


//
// G_DoCompleted 
//
boolean   secretexit; 
extern char* pagename; 

void G_ExitLevel (void) 
{ 
  secretexit = false; 
  gameaction = ga_completed; 
} 

// Here's for the german edition.
void G_SecretExitLevel (void) 
{ 
  // IF NO WOLF3D LEVELS, NO SECRET EXIT!
  if ( (gamemode == commercial) && (W_CheckNumForName("map31")<0))
    secretexit = false;
  else
    secretexit = true; 
  gameaction = ga_completed; 
} 

void G_DoCompleted (void) 
{ 
  //int             i; 

  gameaction = ga_nothing; 

  //for (i=0 ; i<MAXPLAYERS ; i++) 
  if (playeringame[0]) 
    G_PlayerFinishLevel (0);        // take away cards and stuff 

  if (automapactive) 
    AM_Stop ();

  if ( gamemode != commercial)
    switch(gamemap)
  {
    case 8:
      gameaction = ga_victory;
      return;
    case 9: 
      //for (i=0 ; i<MAXPLAYERS ; i++) 
      players[0].didsecret = true; 
      break;
  }

  if ( (gamemap == 8) && (gamemode != commercial) ) 
  {
    // victory 
    gameaction = ga_victory; 
    return; 
  } 

  if ( (gamemap == 9) && (gamemode != commercial) ) 
  {
    // exit secret level 
    //for (i=0 ; i<MAXPLAYERS ; i++) 
    players[0].didsecret = true; 
  }


  wminfo.didsecret = players[consoleplayer].didsecret; 
  wminfo.epsd = gameepisode -1; 
  wminfo.last = gamemap -1;

  // wminfo.next is 0 biased, unlike gamemap
  if ( gamemode == commercial)
  {
    if (secretexit)
      switch(gamemap)
    {
      case 15: wminfo.next = 30; break;
      case 31: wminfo.next = 31; break;
    }
    else
      switch(gamemap)
    {
      case 31:
      case 32: wminfo.next = 15; break;
      default: wminfo.next = gamemap;
    }
  }
  else
  {
    if (secretexit) 
      wminfo.next = 8;  // go to secret level 
    else if (gamemap == 9) 
    {
      // returning from secret level 
      switch (gameepisode) 
      { 
      case 1: 
        wminfo.next = 3; 
        break; 
      case 2: 
        wminfo.next = 5; 
        break; 
      case 3: 
        wminfo.next = 6; 
        break; 
      case 4:
        wminfo.next = 2;
        break;
      }                
    } 
    else 
      wminfo.next = gamemap;          // go to next level 
  }

  wminfo.maxkills = totalkills; 
  wminfo.maxitems = totalitems; 
  wminfo.maxsecret = totalsecret; 
  wminfo.maxfrags = 0; 

  if ( gamemode == commercial )
    wminfo.partime = 35*cpars[gamemap-1]; 
  else
    wminfo.partime = 35*pars[gameepisode][gamemap]; 
  wminfo.pnum = consoleplayer; 

  /*for (i=0 ; i<MAXPLAYERS ; i++) 
  { */
  wminfo.plyr[0].in = playeringame[0]; 
  wminfo.plyr[0].skills = players[0].killcount; 
  wminfo.plyr[0].sitems = players[0].itemcount; 
  wminfo.plyr[0].ssecret = players[0].secretcount; 
  wminfo.plyr[0].stime = leveltime; 
  memcpy (wminfo.plyr[0].frags, players[0].frags, sizeof(wminfo.plyr[0].frags)); 
  //} 

  gamestate = GS_INTERMISSION; 
  viewactive = false; 
  automapactive = false; 

  if (statcopy)
    memcpy (statcopy, &wminfo, sizeof(wminfo));

  WI_Start (&wminfo);
} 


//
// G_WorldDone 
//
void G_WorldDone (void) 
{ 
  gameaction = ga_worlddone; 

  if (secretexit) 
    players[consoleplayer].didsecret = true; 

  if ( gamemode == commercial )
  {
    switch (gamemap)
    {
    case 15:
    case 31:
      if (!secretexit)
        break;
    case 6:
    case 11:
    case 20:
    case 30:
      F_StartFinale ();
      break;
    }
  }
} 

void G_DoWorldDone (void) 
{        
  gamestate = GS_LEVEL; 
  gamemap = wminfo.next+1; 
  G_DoLoadLevel (); 
  gameaction = ga_nothing; 
  viewactive = true; 
} 



//
// G_InitFromSavegame
// Can be called by the startup code or the menu task. 
//
extern boolean setsizeneeded;
void R_ExecuteSetViewSize (void);

#define VERSIONSIZE  16 


void G_DoLoadGame (void) 
{
} 


//
// G_SaveGame
// Called by the menu task.
// Description is a 24 byte text string 
//
void G_SaveGame( int slot, char* description ) 
{ 
} 

void G_DoSaveGame (void)
{ 
} 


//
// G_InitNew
// Can be called by the startup code or the menu task,
// consoleplayer, displayplayer, playeringame[] should be set. 
//
skill_t d_skill; 
int     d_episode; 
int     d_map; 

void G_DeferedInitNew( skill_t skill, int  episode, int  map) 
{ 
  d_skill = skill; 
  d_episode = episode; 
  d_map = map; 
  gameaction = ga_newgame; 
} 


void G_DoNewGame (void) 
{
//  netdemo = false;
//  netgame = false;
  deathmatch = false;
  playeringame[0] = true;
  playeringame[1] = playeringame[2] = playeringame[3] = 0;
  respawnparm = false;
  fastparm = false;
  nomonsters = false;
//  consoleplayer = 0;
  G_InitNew (d_skill, d_episode, d_map);
  gameaction = ga_nothing; 
} 

// The sky texture to be used instead of the F_SKY1 dummy.
extern  int skytexture; 


void G_InitNew ( skill_t skill, int episode, int map ) 
{ 
  //int             i; 

  if (paused) 
  { 
    paused = false; 
  }


  if (skill > sk_nightmare) 
    skill = sk_nightmare;


  // This was quite messy with SPECIAL and commented parts.
  // Supposedly hacks to make the latest edition work.
  // It might not work properly.
  if (episode < 1)
    episode = 1; 

  if ( gamemode == retail )
  {
    if (episode > 4)
      episode = 4;
  }
  else if ( gamemode == shareware )
  {
    if (episode > 1) 
      episode = 1; // only start episode 1 on shareware
  }  
  else
  {
    if (episode > 3)
      episode = 3;
  }



  if (map < 1) 
    map = 1;

  if ( (map > 9) && ( gamemode != commercial) )
    map = 9; 

  M_ClearRandom (); 

  if (skill == sk_nightmare)
    respawnmonsters = true;
  else
    respawnmonsters = false;
  //CGD: make mobjinfo const

  // force players to be initialized upon first level load         
  /*for (i=0 ; i<MAXPLAYERS ; i++) 
  players[i].playerstate = PST_REBORN; */
  players[0].playerstate = PST_REBORN;

  usergame = true;                // will be set false if a demo 
  paused = false; 
  automapactive = false; 
  viewactive = true; 
  gameepisode = episode; 
  gamemap = map; 
  gameskill = skill; 

  viewactive = true;

  // set the sky map for the episode
  if ( gamemode == commercial)
  {
    skytexture = R_TextureNumForName ("SKY3");
    if (gamemap < 12)
      skytexture = R_TextureNumForName ("SKY1");
    else
      if (gamemap < 21)
        skytexture = R_TextureNumForName ("SKY2");
  }
  else
    switch (episode) 
  { 
    case 1: 
      skytexture = R_TextureNumForName ("SKY1"); 
      break; 
    case 2: 
      skytexture = R_TextureNumForName ("SKY2"); 
      break; 
    case 3: 
      skytexture = R_TextureNumForName ("SKY3"); 
      break; 
    case 4: // Special Edition sky
      skytexture = R_TextureNumForName ("SKY4");
      break;
  } 

  G_DoLoadLevel (); 
} 


//
// DEMO RECORDING 
// 
#define DEMOMARKER  0x80


void G_ReadDemoTiccmd (ticcmd_t* cmd) 
{ 
  if (*demo_p == DEMOMARKER) 
  {
    // end of demo data stream 
    G_CheckDemoStatus (); 
    return; 
  } 
  cmd->forwardmove = ((signed char)*demo_p++); 
  cmd->sidemove = ((signed char)*demo_p++); 
  cmd->angleturn = ((unsigned char)*demo_p++)<<8; 
  cmd->buttons = (unsigned char)*demo_p++; 
} 


void G_WriteDemoTiccmd (ticcmd_t* cmd) 
{ 
  if (gamekeydown['q'])           // press q to end demo recording 
    G_CheckDemoStatus (); 
  *demo_p++ = (byte)cmd->forwardmove; 
  *demo_p++ = (byte)cmd->sidemove; 
  *demo_p++ = (byte)((cmd->angleturn+128)>>8); 
  *demo_p++ = (byte)cmd->buttons; 
  demo_p -= 4; 
  if (demo_p > demoend - 16)
  {
    // no more space 
    G_CheckDemoStatus (); 
    return; 
  } 

  G_ReadDemoTiccmd (cmd);         // make SURE it is exactly the same 
} 



//
// G_RecordDemo 
// 


//
// G_PlayDemo 
//

char* defdemoname; 

void G_DeferedPlayDemo (char* name) 
{ 
  defdemoname = name; 
  gameaction = ga_playdemo; 
} 

//
// G_TimeDemo 
//
void G_TimeDemo (char* name) 
{   
  nodrawers = 0; 
  noblit = 0; 
  timingdemo = true; 
  singletics = true; 

  defdemoname = name; 
  gameaction = ga_playdemo; 
} 


/* 
=================== 
= 
= G_CheckDemoStatus 
= 
= Called after a death or level completion to allow demos to be cleaned up 
= Returns true if a new demo loop action will take place 
=================== 
*/ 

boolean G_CheckDemoStatus (void) 
{ 
  int             endtime; 

  if (timingdemo) 
  { 
    endtime = I_GetTime (); 
    I_Error ("timed %i gametics in %i realtics",gametic 
      , endtime-starttime); 
  } 

  return false; 
} 



