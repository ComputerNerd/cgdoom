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
// DESCRIPTION:
//	Gamma correction LUT.
//	Functions to draw patches (by post) directly to screen.
//	Functions to blit a block to the screen.
//
//-----------------------------------------------------------------------------


#ifndef __V_VIDEO__
#define __V_VIDEO__

#include "doomtype.h"

#include "doomdef.h"

// Needed because we are refering to patches.
#include "r_data.h"

// Needed for I_PrepScreen()
#include "i_video.h"

//
// VIDEO
//

#define CENTERY			(SCREENHEIGHT/2)

// Screen 0 is the screen updated by I_Update screen.
// Screen 1 is an extra buffer.

// Background and foreground screen numbers
//
#define BG 1	//4
#define FG 0


#define SCREEN_COUNT	2
extern	byte*	screens[SCREEN_COUNT];

extern  int	dirtybox[4];

extern	const byte	gammatable[5][256];
extern	int	usegamma;



// Allocates buffer screens, call before R_Init.
void V_Init (void);


void
V_CopyRect
( int		srcx,
  int		srcy,
  int		srcscrn,
  int		width,
  int		height,
  int		destx,
  int		desty,
  int		destscrn );

void V_DrawPatch(int x,int y, int scrn,const patch_t*	patch);
#define V_DrawPatchDirect(x,y,scrn,patch ) V_DrawPatch(x,y,scrn,patch)


// Draw a linear block of pixels into the view buffer.
void V_DrawBlock( int x, int y,int scrn,int width,int height,const byte* src );

// Reads a linear block of pixels into the view buffer.
void V_GetBlock(int x,int y,int scrn,int width,int height,byte* dest );


void
V_MarkRect
( int		x,
  int		y,
  int		width,
  int		height );

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
