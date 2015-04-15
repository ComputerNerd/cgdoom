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
//	Archiving: SaveGame I/O.
//
//-----------------------------------------------------------------------------

//static const char 

#include "i_system.h"
#include "z_zone.h"
#include "p_local.h"

// State.
#include "doomstat.h"
#include "r_state.h"

byte*		save_p;


// Pads save_p to a 4-byte boundary
//  so that the load/save works on SGI&Gecko.
#define PADSAVEP()	save_p += (4 - ((int) save_p & 3)) & 3



//
// P_ArchivePlayers
//
void P_ArchivePlayers (void)
{
  int		i;
  int		j;
  player_t*	dest;

  for (i=0 ; i<MAXPLAYERS ; i++)
  {
    if (!playeringame[i])
      continue;

    PADSAVEP();

    dest = (player_t *)save_p;
    memcpy (dest,&players[i],sizeof(player_t));
    save_p += sizeof(player_t);
    for (j=0 ; j<NUMPSPRITES ; j++)
    {
      if (dest->psprites[j].state)
      {
        dest->psprites[j].state 
          = (state_t *)(dest->psprites[j].state-states);
      }
    }
  }
}



//
// P_ArchiveSpecials
//
enum
{
  tc_ceiling,
  tc_door,
  tc_floor,
  tc_plat,
  tc_flash,
  tc_strobe,
  tc_glow,
  tc_endspecials

} specials_e;	



//
// Things to handle:
//
// T_MoveCeiling, (ceiling_t: sector_t * swizzle), - active list
// T_VerticalDoor, (vldoor_t: sector_t * swizzle),
// T_MoveFloor, (floormove_t: sector_t * swizzle),
// T_LightFlash, (lightflash_t: sector_t * swizzle),
// T_StrobeFlash, (strobe_t: sector_t *),
// T_Glow, (glow_t: sector_t *),
// T_PlatRaise, (plat_t: sector_t *), - active list
//
void P_ArchiveSpecials (void)
{
  thinker_t*		th;
  ceiling_t*		ceiling;
  vldoor_t*		door;
  floormove_t*	floor;
  plat_t*		plat;
  lightflash_t*	flash;
  strobe_t*		strobe;
  glow_t*		glow;
  int			i;

  // save off the current thinkers
  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
  {
    if (th->function == 0)
    {
      for (i = 0; i < MAXCEILINGS;i++)
        if (activeceilings[i] == (ceiling_t *)th)
          break;

      if (i<MAXCEILINGS)
      {
        *save_p++ = tc_ceiling;
        PADSAVEP();
        ceiling = (ceiling_t *)save_p;
        memcpy (ceiling, th, sizeof(*ceiling));
        save_p += sizeof(*ceiling);
        ceiling->sector = (sector_t *)(ceiling->sector - sectors);
      }
      continue;
    }

    if (th->function == TT_MoveCeiling)
    {
      *save_p++ = tc_ceiling;
      PADSAVEP();
      ceiling = (ceiling_t *)save_p;
      memcpy (ceiling, th, sizeof(*ceiling));
      save_p += sizeof(*ceiling);
      ceiling->sector = (sector_t *)(ceiling->sector - sectors);
      continue;
    }

    if (th->function == TT_VerticalDoor)
    {
      *save_p++ = tc_door;
      PADSAVEP();
      door = (vldoor_t *)save_p;
      memcpy (door, th, sizeof(*door));
      save_p += sizeof(*door);
      door->sector = (sector_t *)(door->sector - sectors);
      continue;
    }

    if (th->function == TT_MoveFloor)
    {
      *save_p++ = tc_floor;
      PADSAVEP();
      floor = (floormove_t *)save_p;
      memcpy (floor, th, sizeof(*floor));
      save_p += sizeof(*floor);
      floor->sector = (sector_t *)(floor->sector - sectors);
      continue;
    }

    if (th->function == TT_PlatRaise)
    {
      *save_p++ = tc_plat;
      PADSAVEP();
      plat = (plat_t *)save_p;
      memcpy (plat, th, sizeof(*plat));
      save_p += sizeof(*plat);
      plat->sector = (sector_t *)(plat->sector - sectors);
      continue;
    }

    if (th->function == TT_LightFlash)
    {
      *save_p++ = tc_flash;
      PADSAVEP();
      flash = (lightflash_t *)save_p;
      memcpy (flash, th, sizeof(*flash));
      save_p += sizeof(*flash);
      flash->sector = (sector_t *)(flash->sector - sectors);
      continue;
    }

    if (th->function == TT_StrobeFlash)
    {
      *save_p++ = tc_strobe;
      PADSAVEP();
      strobe = (strobe_t *)save_p;
      memcpy (strobe, th, sizeof(*strobe));
      save_p += sizeof(*strobe);
      strobe->sector = (sector_t *)(strobe->sector - sectors);
      continue;
    }

    if (th->function == TT_Glow)
    {
      *save_p++ = tc_glow;
      PADSAVEP();
      glow = (glow_t *)save_p;
      memcpy (glow, th, sizeof(*glow));
      save_p += sizeof(*glow);
      glow->sector = (sector_t *)(glow->sector - sectors);
      continue;
    }
  }

  // add a terminating marker
  *save_p++ = tc_endspecials;	

}


//
// P_UnArchiveSpecials
//
void P_UnArchiveSpecials (void)
{
  byte		tclass;
  ceiling_t*		ceiling;
  vldoor_t*		door;
  floormove_t*	floor;
  plat_t*		plat;
  lightflash_t*	flash;
  strobe_t*		strobe;
  glow_t*		glow;


  // read in saved thinkers
  for(;;)
  {
    tclass = *save_p++;
    switch (tclass)
    {
    case tc_endspecials:
      return;	// end of list

    case tc_ceiling:
      PADSAVEP();
      ceiling = (ceiling_t*)Z_Malloc (sizeof(*ceiling), PU_LEVEL, NULL);
      memcpy (ceiling, save_p, sizeof(*ceiling));
      save_p += sizeof(*ceiling);
      ceiling->sector = &sectors[(int)ceiling->sector];
      ceiling->sector->specialdata = ceiling;

      if (ceiling->thinker.function)
        ceiling->thinker.function = TT_MoveCeiling;

      P_AddThinker (&ceiling->thinker,ceiling);
      P_AddActiveCeiling(ceiling);
      break;

    case tc_door:
      PADSAVEP();
      door = (vldoor_t*)Z_Malloc (sizeof(*door), PU_LEVEL, NULL);
      memcpy (door, save_p, sizeof(*door));
      save_p += sizeof(*door);
      door->sector = &sectors[(int)door->sector];
      door->sector->specialdata = door;
      door->thinker.function = TT_VerticalDoor;
      P_AddThinker (&door->thinker,door);
      break;

    case tc_floor:
      PADSAVEP();
      floor = (floormove_t*)Z_Malloc (sizeof(*floor), PU_LEVEL, NULL);
      memcpy (floor, save_p, sizeof(*floor));
      save_p += sizeof(*floor);
      floor->sector = &sectors[(int)floor->sector];
      floor->sector->specialdata = floor;
      floor->thinker.function = TT_MoveFloor;
      P_AddThinker (&floor->thinker,floor);
      break;

    case tc_plat:
      PADSAVEP();
      plat = (plat_t*)Z_Malloc (sizeof(*plat), PU_LEVEL, NULL);
      memcpy (plat, save_p, sizeof(*plat));
      save_p += sizeof(*plat);
      plat->sector = &sectors[(int)plat->sector];
      plat->sector->specialdata = plat;

      if (plat->thinker.function)
        plat->thinker.function = TT_PlatRaise;

      P_AddThinker (&plat->thinker,plat);
      P_AddActivePlat(plat);
      break;

    case tc_flash:
      PADSAVEP();
      flash = (lightflash_t*)Z_Malloc (sizeof(*flash), PU_LEVEL, NULL);
      memcpy (flash, save_p, sizeof(*flash));
      save_p += sizeof(*flash);
      flash->sector = &sectors[(int)flash->sector];
      flash->thinker.function = TT_LightFlash;
      P_AddThinker (&flash->thinker,flash);
      break;

    case tc_strobe:
      PADSAVEP();
      strobe = (strobe_t*)Z_Malloc (sizeof(*strobe), PU_LEVEL, NULL);
      memcpy (strobe, save_p, sizeof(*strobe));
      save_p += sizeof(*strobe);
      strobe->sector = &sectors[(int)strobe->sector];
      strobe->thinker.function = TT_StrobeFlash;
      P_AddThinker (&strobe->thinker,strobe);
      break;

    case tc_glow:
      PADSAVEP();
      glow = (glow_t*)Z_Malloc (sizeof(*glow), PU_LEVEL, NULL);
      memcpy (glow, save_p, sizeof(*glow));
      save_p += sizeof(*glow);
      glow->sector = &sectors[(int)glow->sector];
      glow->thinker.function = TT_Glow;
      P_AddThinker (&glow->thinker,glow);
      break;

    default:
      I_Error ("P_UnarchiveSpecials:Unknown tclass %i in savegame",tclass);
    }

  }
}

