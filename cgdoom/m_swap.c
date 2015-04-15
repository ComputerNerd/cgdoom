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
//	Endianess handling, swapping 16bit and 32bit.
//
//-----------------------------------------------------------------------------

//static const char 
#include "os.h"

#ifdef __GNUG__
#pragma implementation "m_swap.h"
#endif
#include "m_swap.h"

#ifndef CG_EMULATOR
#define __BIG_ENDIAN__
#endif 


// Swap 16bit, that is, MSB and LSB byte.
unsigned short SwapSHORT(const unsigned short x)
{
#ifdef __BIG_ENDIAN__
    // No masking with 0xFF should be necessary. 
    return (x>>8) | (x<<8);
#else
    return x;
#endif 
}

// Swapping 32bit.
unsigned long SwapLONG(const unsigned long x)
{
#ifdef __BIG_ENDIAN__
    return
	(x>>24)
	| ((x>>8) & 0xff00)
	| ((x<<8) & 0xff0000)
	| (x<<24);
#else
    return x;
#endif 

}

unsigned short SHORTFromBytes(const unsigned char *pData)
{
  unsigned short w = pData[1];
  w <<= 8;
  w |= pData[0];
  return w;
}

unsigned int INTFromBytes(const unsigned char *pData)
{
  unsigned int i = pData[3];
  i <<= 8;
  i |= pData[2];
  i <<= 8;
  i |= pData[1];
  i <<= 8;
  i |= pData[0];
  return i;
}

