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
//	WAD I/O functions.
//
//-----------------------------------------------------------------------------


#ifndef __W_WAD__
#define __W_WAD__


#ifdef __GNUG__
#pragma interface
#endif


#include "r_defs.h"
//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char		identification[4];		
    int			numlumps;
    int			infotableofs;
    
} wadinfo_t;


typedef struct
{
    int			filepos;
    int			size;
    char		name[8];
    
} filelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char	name[8];
//    int		handle;
    int		position;
    int		size;
} lumpinfo_t;


extern	void**		lumpcache;
extern	lumpinfo_t*	lumpinfo;

int    W_InitMultipleFiles (void);
//void    W_Reload (void);

int		W_CheckNumForName (const char* name);
int		W_GetNumForName (const char* name);

int		W_LumpLength (int lump);
void    W_ReadLump (int lump, void *dest);
void*   W_ReadLumpWithZ_Malloc(int lump,int tag,int iEnableFlash);

//void* W_CacheLumpNum (int lump, int tag);
void *W_CacheLumpNumVolatile(int lump, int tag);
const void* W_CacheLumpNumConst (int lump, int tag);

const patch_t *W_CacheLumpNumPatch(int lump, int tag);

//void* W_CacheLumpName (const char* name, int tag);
void* W_CacheLumpNameVolatile (const char* name, int tag);
const void* W_CacheLumpNameConst (const char* name, int tag);
const patch_t *W_CacheLumpNamePatch(const char* name, int tag);



//void strupr (char* s);


#endif
//-----------------------------------------------------------------------------
//
// $Log:$
//
//-----------------------------------------------------------------------------
