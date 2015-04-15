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
//	Thinker, Ticker.
//
//-----------------------------------------------------------------------------

//static const char 


#include "z_zone.h"
#include "p_local.h"

#include "doomstat.h"

#include "i_system.h"


int	leveltime;

//
// THINKERS
// All thinkers should be allocated by Z_Malloc
// so they can be operated on uniformly.
// The actual structures will vary in size,
// but the first element must be thinker_t.
//



// Both the head and tail of the thinker list.
thinker_t	thinkercap;


//
// P_InitThinkers
//
void P_InitThinkers (void)
{
  thinkercap.prev = thinkercap.next  = &thinkercap;
}




//
// P_AddThinker
// Adds a new thinker at the end of the list.
//
void P_AddThinker (thinker_t* thinker, void* tgt)
{
  thinkercap.prev->next = thinker;
  thinker->next = &thinkercap;
  thinker->prev = thinkercap.prev;
  thinker->target= tgt;
  thinkercap.prev = thinker;
}

//
// P_RemoveThinker
// Deallocation is lazy -- it will not actually be freed
// until its thinking turn comes up.
//
void P_RemoveThinker (thinker_t* thinker)
{
  // FIXME: NOP.
  thinker->function = TT_Ded;
}



/*//
// P_AllocateThinker
// Allocates memory and adds a new thinker at the end of the list.
//
void P_AllocateThinker (thinker_t*	thinker)
{
}*/



//
// P_RunThinkers
//
void P_RunThinkers (void)
{
  thinker_t*	currentthinker;

  currentthinker = thinkercap.next;
  while (currentthinker != &thinkercap)
  {
    if ( currentthinker->function == TT_Ded )
    {
      //I_Error("fucks, thinker died");
      // time to remove it
      currentthinker->next->prev = currentthinker->prev;
      currentthinker->prev->next = currentthinker->next;
      Z_Free (currentthinker);
    }
    else
    {
      // If action not null...
      if (currentthinker->function)
      {
        // ...execute its action (since we've removed the function pointers, we're doing it in an old fashioned way...)
        switch (currentthinker->function)
        {
        case TP_MobjThinker:P_MobjThinker((mobj_t*)currentthinker->target);break;
        case TT_MoveCeiling:T_MoveCeiling((ceiling_t*)currentthinker->target);break;
        case TT_VerticalDoor:T_VerticalDoor((vldoor_t*)currentthinker->target);break;
        case TT_PlatRaise:T_PlatRaise((plat_t*)currentthinker->target);break;
        case TT_MoveFloor:T_MoveFloor((floormove_t*)currentthinker->target);break;
        case TT_LightFlash:T_LightFlash((lightflash_t*)currentthinker->target);break;
        case TT_StrobeFlash:T_StrobeFlash((strobe_t*)currentthinker->target);break;
        case TT_Glow:T_Glow((glow_t*)currentthinker->target);break;
        case TT_FireFlicker:T_FireFlicker((fireflicker_t*)currentthinker->target);break;
        default:
          //printf("func= %i \n",currentthinker->function);
          I_Error("UNDEFINED OBJ FUNCTION");
          break;
        }
        //I_Error("TSB");
        //currentthinker->function (currentthinker);
        //currentthinker->function (currentthinker,NULL);
      }
    }
    currentthinker = currentthinker->next;
  }
}



//
// P_Ticker
//

void P_Ticker (void)
{

  // run the tic
  if (paused)
    return;

  // pause if in menu and at least one tic has been run
  if ( menuactive && players[consoleplayer].viewz != 1)
  {
    return;
  }

  if (playeringame[0])
    P_PlayerThink (&players[0]);

  P_RunThinkers ();

  P_UpdateSpecials ();

  P_RespawnSpecials ();

  // for par times
  leveltime++;	
}
