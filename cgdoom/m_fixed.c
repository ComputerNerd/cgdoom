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
//	Fixed point implementation.
//
//-----------------------------------------------------------------------------


//static const char 

#include "os.h"

#include "doomtype.h"
#include "i_system.h"

#ifdef __GNUG__
#pragma implementation "m_fixed.h"
#endif
#include "m_fixed.h"


// Fixme. __USE_C_FIXED__ or something.
//CGD: Bastl impl of int*int=>int64 , (int64 >>16) => int
fixed_t FixedMul( fixed_t	a, fixed_t	b )
{
  int iResult;
  unsigned int iNegative = (a<0) != (b <0);
  unsigned int iMask = 1;
  unsigned int iResLo = 0;
  unsigned int iResHi = 0;
  unsigned int iaHi = 0;
  unsigned int ua = abs(a);
  unsigned int ub = abs(b);
  if(ua<ub)	//swap
  {
    unsigned int uc = ua;
    ua = ub;
    ub = uc;
  }
  ASSERT((ub & 0x80000000)==0);
  while(ub)
  {
    if(ub & iMask)
    {
      unsigned int iLo = iResLo + ua;
      if(iLo < iResLo)
      {
        iResHi ++;
      }
      iResLo = iLo;
      iResHi += iaHi;
      ub &= ~iMask;
    }
    iMask <<= 1;
    iaHi <<= 1;
    if(ua & 0x80000000)
    {
      iaHi |= 1;
    }
    ua <<= 1;
  }
#ifdef CG_EMULATOR
  ASSERT(FRACBITS == 16);
  {
    long long c = iResHi;
    c <<= 32;
    c |= iResLo;
    if(iNegative) c= -c;
    ASSERT(c == (((long long)a) * ((long long) b)));
  }
#endif //#ifdef CG_EMULATOR
  iResLo >>= FRACBITS;
  iResHi <<= FRACBITS;
  iResult = iResHi | iResLo;
  if(iNegative) iResult= -iResult;
#ifdef CG_EMULATOR
  {
    fixed_t y =(int)((((long long)a) * ((long long) b)) >> FRACBITS); 
    ASSERT(abs(iResult-y)<=1);
  }
#endif //#ifdef CG_EMULATOR
  return iResult;
}


//
// FixedDiv, C version.
//

unsigned int FixedDivU(unsigned int ua, unsigned int ub )
{
  unsigned int uRes = 0;
  int i;
  for(i=0;i<FRACBITS;i++)
  {
    unsigned int uDiv = ua / ub;
    ua = ua % ub;
    uRes += uDiv;
    uRes <<= 1;
    ua <<= 1;
  }
  return uRes;
}


fixed_t FixedDiv( fixed_t	a, fixed_t	b )
{
  int iResult;
  int iNegative = (a < 0) != (b<0);
  if ( (abs(a)>>14) >= abs(b))
    return iNegative ? MININT : MAXINT;
  iResult = FixedDivU(abs(a),abs(b));
  if(iNegative) iResult = -iResult;
#ifdef CG_EMULATOR
  ASSERT(abs(iResult-((long long)a<<16) /((long long)b))<2);
#endif //#ifdef CG_EMULATOR
  return iResult;
}
