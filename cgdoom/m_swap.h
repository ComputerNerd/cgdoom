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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------


#ifndef __M_SWAP__
#define __M_SWAP__


#ifdef __GNUG__
#pragma interface
#endif

// Endianess handling.
// WAD files are stored little endian.
unsigned short	SwapSHORT(const unsigned short);
unsigned long	SwapLONG(const unsigned long);

unsigned short SHORTFromBytes(const unsigned char *pData);
unsigned int INTFromBytes(const unsigned char *pData);

#define SHORT_NORM(x)	((short)SwapSHORT((unsigned short) (x)))
#define LONG(x)         ((long)SwapLONG((unsigned long) (x)))

//CGD: when accessing flash (directly) words/longs may not be aligned => crash
// so load all per bytes
#define SHORT(x) ((short)SHORTFromBytes((const unsigned char*)&(x##LE)))

#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
