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
// Handles WAD file header, directory, lump I/O.
//
//-----------------------------------------------------------------------------


//static const char 


/*#ifdef NORMALUNIX
#include <ctype.h>
#include <sys/types.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <alloca.h>
#define O_BINARY  0
#endif*/

#include "os.h"

#include "doomtype.h"
#include "m_swap.h"
#include "i_system.h"
#include "z_zone.h"

#ifdef __GNUG__
#pragma implementation "w_wad.h"
#endif
#include "w_wad.h"




//
// GLOBALS
//

// Location of each lump on disk.
lumpinfo_t*  lumpinfo;
static int  numlumps;

void**   lumpcache;


//
// LUMP BASED ROUTINES.
//

//
// W_AddFile
// All files are optional, but at least one file must be
//  found (PWAD, if all required lumps are present).
// Files with a .wad extension are wadlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
//
// If filename starts with a tilde, the file is handled
//  specially to allow map reloads.
// But: the reload feature is a fragile hack...
// THAT'S WHY RELOADABLE FILE SUPPORT WAS REMOVED, YOU FUCKERS! STOP USING CRAPPY HACKS!

static int W_AddFile ()
{
  wadinfo_t  header;
  lumpinfo_t*  lump_p;
  int    i;
  int    length;
  int    startlump;
  filelump_t*  fileinfo;
  filelump_t*  fileinfo_mem;

  if (gWADHandle < 0)
  {
    I_Error("Couldn't open doom.wad");
    return 0; // CX port
  }

  //printf ("Adding %s\n",filename);
  startlump = numlumps;

  //Bfile_ReadFile_OS(gWADHandle, &header, sizeof(header),0);
  Flash_ReadFile(&header, sizeof(header),0);
  if (CGDstrncmp(header.identification,"IWAD",4))
  {
    // Homebrew levels?
    if (CGDstrncmp(header.identification,"PWAD",4))
      return 0; // CX port
    //I_Error ("Wad file %s doesn't have IWAD or PWAD id\n", filename);
  }
  header.numlumps = LONG(header.numlumps);
  header.infotableofs = LONG(header.infotableofs);
  length = header.numlumps * sizeof(filelump_t);
  fileinfo = fileinfo_mem= (filelump_t *)CGDMalloc(length);
  //Bfile_ReadFile_OS(gWADHandle, fileinfo, length,header.infotableofs);
  Flash_ReadFile(fileinfo, length,header.infotableofs);
  numlumps += header.numlumps;

  // Fill in lumpinfo
  lumpinfo = (lumpinfo_t *)CGDRealloc (lumpinfo, numlumps*sizeof(lumpinfo_t));

  if (!lumpinfo)
    I_Error ("Couldn't realloc lumpinfo");

  lump_p = &lumpinfo[startlump];

  for (i=startlump ; i<numlumps ; i++,lump_p++, fileinfo++)
  {
    //        lump_p->handle = handle; //Originally, this was storehandle... hopefully this won't break anything...
    lump_p->position = LONG(fileinfo->filepos);
    lump_p->size = LONG(fileinfo->size);
    memset(lump_p->name,0,8);//hack
    CGDstrncpy (lump_p->name, fileinfo->name, 8);
  }

  //Nspire has no alloca() function for temporary memory, so we have to use malloc() and free() to compensate
  free(fileinfo_mem);
  return 1; // CX port
}

//
// W_InitMultipleFiles
// Pass a null terminated list of files to use.
// All files are optional, but at least one file
//  must be found.
// Files with a .wad extension are idlink files
//  with multiple lumps.
// Other files are single lumps with the base filename
//  for the lump name.
// Lump names can appear multiple times.
// The name searcher looks backwards, so a later file
//  does override all earlier ones.
//
int W_InitMultipleFiles(void)
{
  // open all the files, load headers, and count lumps
  numlumps = 0;

  // will be realloced as lumps are added
  //lumpinfo = (lumpinfo_t*)CGDMalloc(1); 
  //CGD: CGDrealloc accepts NULL
  lumpinfo = NULL;
  if(!W_AddFile ()) 
  {
    return 0; // CX port
  }

  if (!numlumps)
  {
    I_Error ("W_InitFiles: no files found");
  }

  // set up caching
  //printf ("numlumps = %i \n",numlumps);
  lumpcache = (void **)CGDCalloc(numlumps * sizeof(*lumpcache));

  if (!lumpcache)
  {
    I_Error ("Couldn't allocate lumpcache");
  }

  return 1; // CX port
}


//
// W_NumLumps
//
int W_NumLumps (void)
{
  return numlumps;
}



//
// W_CheckNumForName
// Returns -1 if name not found.
//

int W_CheckNumForName (const char* name)
{
  lumpinfo_t* lump_p;
  // scan backwards so patch lump files take precedence
  lump_p = lumpinfo + numlumps;

  while(lump_p-- != lumpinfo)
  {
    if(CGDstrnicmp(name,lump_p->name,8)==0 )
    {
      return lump_p - lumpinfo;
    }
  }
  // TFB. Not found.
  return -1;
}




//
// W_GetNumForName
// Calls W_CheckNumForName, but bombs out if not found.
//
int W_GetNumForName (const char* name)
{
  int i;
  //printf("w_getnumforname(%s) called \n",name);
  i = W_CheckNumForName (name);

  if (i == -1)
  {
#ifdef CG_EMULATOR
    printf("Accessing %s \n",name);
#endif //#ifdef CG_EMULATOR
    I_Error ("W_GetNumForName: %s not found!", name);
  }

  return i;
}


//
// W_LumpLength
// Returns the buffer size needed to load the given lump.
//
int W_LumpLength (int lump)
{
  if (lump >= numlumps)
    I_Error ("W_LumpLength: %i >= numlumps",lump);

  return lumpinfo[lump].size;
}



//
// W_ReadLump
// Loads the lump into the given buffer,
//  which must be >= W_LumpLength().
//
void W_ReadLump(int lump, void* dest )
{
  int     c;
  lumpinfo_t* l;

  if (lump >= numlumps)
  {
    I_Error ("W_ReadLump: %i >= numlumps",lump);
  }

  l = lumpinfo+lump;
  //c = Bfile_ReadFile_OS(gWADHandle,dest, l->size,l->position);
  c = Flash_ReadFile(dest, l->size,l->position);


  /*printf("W_ReadLump(%i,...) in progress... \n",lump);
  printf("l->handle = %i \n",(int)l->handle);
  printf("l->size = %i \n",l->size);
  printf("l->position = %i \n",l->position);
  printf("c= %i \n",c);*/

  if (c < l->size)
  {
    I_ErrorI ("W_ReadLump: only read %i of %i on lump %i", c,l->size,lump,0);//CGD HACK
  }
}


void * W_ReadLumpWithZ_Malloc(int lump,int tag,int iEnableFlash)
{
  int     c = 0;

  if (lump >= numlumps)
  {
    I_Error ("W_ReadLump: %i >= numlumps",lump);
  }
  if(iEnableFlash)
  {
    c = FindInFlash(&lumpcache[lump], lumpinfo[lump].size, lumpinfo[lump].position);
  }
//  else ASSERT(lumpinfo[lump].size != 10240);

  if(c != lumpinfo[lump].size)
  {
    Z_Malloc (lumpinfo[lump].size, tag, &lumpcache[lump]);
    W_ReadLump (lump, lumpcache[lump]);
  }
  return lumpcache[lump];
}


//
// W_CacheLumpNum
//
static void* W_CacheLumpNumCommon( int lump, int tag,int iEnableFlash)
{

  if (lump >= numlumps)
  {
    I_Error ("W_CacheLumpNum: %i >= numlumps (%i)",lump,numlumps);
  }


  //printf("x");

  if (!lumpcache[lump])
  {
    // try to find in flash (may fail if the piece is in 2 fragments (file fragmentaion)
    W_ReadLumpWithZ_Malloc(lump,tag,iEnableFlash);
  }
  else
  {
    //printf ("cache hit on lump %i\n",lump);
    Z_ChangeTag (lumpcache[lump],tag);
  }

  /*if (lump==1203)
  printf("sky access, returning %x \n",lumpcache[lump]);*/

  return lumpcache[lump];
}

//#define USE_FLASH 0

void *W_CacheLumpNumVolatile (int lump, int tag)
{
  return W_CacheLumpNumCommon(lump,tag,0);
}

const void* W_CacheLumpNumConst (int lump, int tag)
{
  return W_CacheLumpNumCommon(lump,tag,1);
}

/*void* W_CacheLumpNum(int lump, int tag)
{
  return W_CacheLumpNumCommon(lump,tag,USE_FLASH);
}*/

const patch_t* W_CacheLumpNumPatch(int lump, int tag)
{
 return (patch_t*) W_CacheLumpNumCommon(lump,tag,1);
}

//
// W_CacheLumpName
//
void* W_CacheLumpNameVolatile (const char* name, int tag)
{
  return W_CacheLumpNumCommon(W_GetNumForName(name),tag,0);
}


/*void* W_CacheLumpName (const char* name, int tag)
{
  return W_CacheLumpNumCommon(W_GetNumForName(name),tag,USE_FLASH);
}*/

const void* W_CacheLumpNameConst (const char* name, int tag)
{
  return W_CacheLumpNumCommon(W_GetNumForName(name),tag,1);
}

const patch_t *W_CacheLumpNamePatch(const char* name, int tag)
{
  return (patch_t *)W_CacheLumpNumCommon(W_GetNumForName(name),tag,1);
}

