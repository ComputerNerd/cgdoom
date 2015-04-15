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
//	Preparation of data for rendering,
//	generation of lookups, caching, retrieval by name.
//
//-----------------------------------------------------------------------------


//static const char 


#include "i_system.h"
#include "z_zone.h"

#include "m_swap.h"

#include "w_wad.h"

#include "doomdef.h"
#include "r_local.h"
#include "p_local.h"

#include "doomstat.h"
#include "r_sky.h"

#include "os.h"

#include "r_data.h"

//
// Graphics.
// DOOM graphics for walls and sprites
// is stored in vertical runs of opaque pixels (posts).
// A column is composed of zero or more posts,
// a patch or sprite is composed of zero or more columns.
// 



//
// Texture definition.
// Each texture is composed of one or more patches,
// with patches being lumps stored in the WAD.
// The lumps are referenced by number, and patched
// into the rectangular texture space using origin
// and possibly other attributes.
//
typedef struct
{
  short			originxLE;
  short			originyLE;
  short			patchLE;
  short			stepdirLE;
  short			colormapLE;
} mappatch_t;


//
// Texture definition.
// A DOOM wall texture is a list of patches
// which are to be combined in a predefined order.
//
typedef struct
{
  char			name[8];
  //boolean			masked;
  int				masked;			//Carmack, you stupid fucknut, int and boolean DO NOT have the same length!!!!
  short			widthLE;
  short			heightLE;
  int				obsolete;
  short			patchcountLE;
  mappatch_t		patches[1];
} maptexture_t;


// A single patch from a texture definition,
//  basically a rectangular area within
//  the texture rectangle.
typedef struct
{
  // Block origin (allways UL),
  // which has allready accounted
  // for the internal origin of the patch.
  short			originx;	
  short			originy;
  int				patch;
} texpatch_t;


// A maptexturedef_t describes a rectangular texture,
//  which is composed of one or more mappatch_t structures
//  that arrange graphic patches.
typedef struct
{
  // Keep name for switch changing, etc.
  char			name[8];		
  short			width;
  short			height;

  // All the patches[patchcount]
  //  are drawn back to front into the cached texture.
  short			patchcount;
  texpatch_t		patches[1];		

} texture_t;



int					firstflat;
int					lastflat;
int					numflats;

int					firstpatch;
int					lastpatch;
int					numpatches;

int					firstspritelump;
int					lastspritelump;
int					numspritelumps;

int					numtextures;
texture_t**			textures;


int*				texturewidthmask;
// needed for texture pegging
fixed_t*			textureheight;		
int*				texturecompositesize;
short**				texturecolumnlump;
unsigned short**	texturecolumnofs;
byte**				texturecomposite;

// for global animation
int*				flattranslation;
int*				texturetranslation;

// needed for pre rendering
fixed_t*			spritewidth;	
fixed_t*			spriteoffset;
fixed_t*			spritetopoffset;

const lighttable_t		*colormaps;


//
// MAPTEXTURE_T CACHING
// When a texture is first needed,
//  it counts the number of composite columns
//  required in the texture and allocates space
//  for a column directory and any new columns.
// The directory will simply point inside other patches
//  if there is only one patch in a given column,
//  but any columns with multiple patches
//  will have new column_ts generated.
//
//
// R_DrawColumnInCache
// Clip and draw a column
//  from a patch into a cached post.
//
static void R_DrawColumnInCache(const column_t* patch, byte* cache, int originy, int cacheheight )
{
  int		count;
  int		position;
  byte*	source;
  byte*	dest;

  dest = (byte *)cache + 3;

  while (patch->topdelta != 0xff)
  {
    source = (byte *)patch + 3;
    count = patch->length;
    position = originy + patch->topdelta;

    if (position < 0)
    {
      count += position;
      position = 0;
    }

    if (position + count > cacheheight)
      count = cacheheight - position;

    if (count > 0)
      memcpy (cache + position, source, count);

    patch = (column_t *)(  (byte *)patch + patch->length + 4);
  }
}

// R_GenerateComposite
// Using the texture definition,
//  the composite texture is created from the patches,
//  and each column is cached.
//
static void R_GenerateComposite (int texnum)
{
  byte*			block;
  texture_t*		texture;
  texpatch_t*		patch;	
  const patch_t*		realpatch;
  int				x;
  int				x1;
  int				x2;
  int				i;
  const column_t*		patchcol;
  short*			collump;
  unsigned short*	colofs;

  texture = textures[texnum];

  block = (byte*)Z_Malloc (texturecompositesize[texnum],PU_STATIC, &texturecomposite[texnum]);

  collump = texturecolumnlump[texnum];
  colofs = texturecolumnofs[texnum];

  // Composite the columns together.
  patch = texture->patches;

  for (i=0 , patch = texture->patches;i<texture->patchcount; i++, patch++)
  {
    realpatch = W_CacheLumpNumPatch(patch->patch, PU_CACHE);
    x1 = patch->originx;
    x2 = x1 + SHORT(realpatch->width);

    if (x1<0)
      x = 0;
    else
      x = x1;

    if (x2 > texture->width)
      x2 = texture->width;

    for ( ; x<x2 ; x++)
    {
      // Column does not have multiple patches?
      if (collump[x] >= 0)
      {
        continue;
      }

      patchcol = (const column_t *)((const byte *)realpatch + INTFromBytes((const byte*)&(realpatch->columnofsLE[x-x1])));
      R_DrawColumnInCache(patchcol,block + colofs[x],patch->originy,texture->height);
    }

  }

  // Now that the texture has been built in column cache,
  //  it is purgable from zone memory.
  Z_ChangeTag (block, PU_CACHE);
}




//
// R_GenerateLookup
//
void R_GenerateLookup (int texnum)
{
  texture_t*		texture;
  texpatch_t*		patch;	
  const patch_t*	realpatch;
  int				x;
  int				x1;
  int				x2;
  int				i;
  short*			collump;
  unsigned short*	colofs;
  byte              patchcount[320];// patchcount[texture->width]

  texture = textures[texnum];

  // Composited texture not created yet.
  texturecomposite[texnum] = 0;

  texturecompositesize[texnum] = 0;
  collump = texturecolumnlump[texnum];
  colofs = texturecolumnofs[texnum];

  // Now count the number of columns
  //  that are covered by more than one patch.
  // Fill in the lump / offset, so columns
  //  with only a single patch are all done.
  //patchcount = (byte *)alloca (texture->width);
  ASSERT(texture->width <= 320);

  memset (patchcount, 0, texture->width);
  patch = texture->patches;

  //printf("[%s, ",texture->name);

  for (i=0 , patch = texture->patches;     i<texture->patchcount;    i++, patch++)
  {
    realpatch = W_CacheLumpNumPatch(patch->patch, PU_CACHE);
    //printf("%i/%i] ",realpatch->leftoffset,patch->originx);
    x1 = patch->originx;
    x2 = x1 + SHORT(realpatch->width);

    if (x1 < 0)
      x = 0;
    else
      x = x1;

    if (x2 > texture->width)
      x2 = texture->width;

    for ( ; x<x2 ; x++)
    {
      patchcount[x]++;
      collump[x] = (short)patch->patch;
      colofs[x] = (unsigned short)(3 + INTFromBytes((const byte*)&(realpatch->columnofsLE[x-x1])));
    }
  }

  for (x=0 ; x<texture->width ; x++)
  {
    if (!patchcount[x])
    {
      //printf ("R_GenerateLookup: column without a patch (%s)\n",texture->name);
      return;
    }
    // I_Error ("R_GenerateLookup: column without a patch");

    if (patchcount[x] > 1)
    {
      // Use the cached block.
      collump[x] = -1;	
      colofs[x] = (unsigned short)texturecompositesize[texnum];

      if (texturecompositesize[texnum] > 0x10000-texture->height)
      {
        I_Error ("R_GenerateLookup: texture %i is >64k",texnum);
      }

      texturecompositesize[texnum] += texture->height;
    }
  }

}




//
// R_GetColumn
//
const byte* R_GetColumn ( int	tex, int col )
{
  int		lump;
  int		ofs;


  col &= texturewidthmask[tex];
  lump = texturecolumnlump[tex][col];
  ofs = texturecolumnofs[tex][col];

  //CGD: hack - this program is not DOOM any more
  //it is cripled doom
  // to save RAM, do not use composite textures, but use just some texture
  //FAIL: switches are not visible, so try to make composite textures only for small sizes
  if((lump <= 0) && (texturecompositesize[tex] > 8192))
  {
    lump = 990;
    ofs = 267;
  }
  if (lump > 0)
    return (const byte *)W_CacheLumpNumConst(lump,PU_CACHE)+ofs;
  
  if (!texturecomposite[tex])
    R_GenerateComposite (tex);

  return texturecomposite[tex] + ofs;
}




//
// R_InitTextures
// Initializes the texture list
//  with the textures from the world map.
//
void R_InitTextures (void)
{
  maptexture_t*	mtexture;
  texture_t*		texture;
  mappatch_t*		mpatch;
  texpatch_t*		patch;

  int				i;
  int				j;

  const char*			maptex;
  const char*			maptex2;
  const char*			maptex1;


  const char*			names;
  const char*			name_p;

  int*			patchlookup;

  int				totalwidth;
  int				nummappatches;
  int				offset;
  int				maxoff;
  int				maxoff2;
  int				numtextures1;
  int				numtextures2;

  const char*			directory;

  /*int				temp1;
  int				temp2;
  int				temp3;*/



  // Load the patch names from pnames.lmp.

  names = (const char *)W_CacheLumpNameConst ("PNAMES", PU_STATIC);
  nummappatches = INTFromBytes((const byte*)names );
  name_p = names+4;
  patchlookup = (int*)CGDMalloc(nummappatches*sizeof(*patchlookup));

  for (i=0 ; i<nummappatches ; i++)
  {
    //CGDstrncpy (name,name_p+i*8, 8);
    //printf ("%i \n",i);
    //printf ("%s \n",name);
    //printf ("%i \n",sizeof(name));
    /*if (i==1)
    {
    printf ("nummappatches = %i \n",nummappatches);
    I_Error("BREAK3","");
    }*/
    patchlookup[i] = W_CheckNumForName (name_p+i*8);
  }

  Z_Free (names);

  // Load the map texture definitions from textures.lmp.
  // The data is contained in one or two lumps,
  //  TEXTURE1 for shareware, plus TEXTURE2 for commercial.
  maptex = maptex1 = (const char *)W_CacheLumpNameConst("TEXTURE1", PU_STATIC);
  numtextures1 = INTFromBytes((const byte*)maptex1);
  maxoff = W_LumpLength (W_GetNumForName ("TEXTURE1"));
  directory = maptex+4;

  if (W_CheckNumForName ("TEXTURE2") != -1)
  {
    maptex2 = (const char*)W_CacheLumpNameConst("TEXTURE2", PU_STATIC);
    numtextures2 = INTFromBytes((const byte*)maptex2);
    maxoff2 = W_LumpLength (W_GetNumForName ("TEXTURE2"));
  }
  else
  {
    maptex2 = NULL;
    numtextures2 = 0;
    maxoff2 = 0;
  }

  numtextures = numtextures1 + numtextures2;

  textures = (texture_t **)Z_Malloc (numtextures*4, PU_STATIC, 0);
  texturecolumnlump = (short**)Z_Malloc (numtextures*4, PU_STATIC, 0);
  texturecolumnofs = (unsigned short**)Z_Malloc (numtextures*4, PU_STATIC, 0);
  texturecomposite = (byte**)Z_Malloc (numtextures*4, PU_STATIC, 0);
  texturecompositesize = (int*)Z_Malloc (numtextures*4, PU_STATIC, 0);
  texturewidthmask = (int*)Z_Malloc (numtextures*4, PU_STATIC, 0);
  textureheight = (fixed_t*)Z_Malloc (numtextures*4, PU_STATIC, 0);

  totalwidth = 0;

  /*//	Really complex printing shit...
  temp1 = W_GetNumForName ("S_START");  // P_???????
  temp2 = W_GetNumForName ("S_END") - 1;
  temp3 = ((temp2-temp1+63)/64) + ((numtextures+63)/64);
  printf("[");
  for (i = 0; i < temp3; i++)
  printf(" ");
  printf("         ]");
  for (i = 0; i < temp3; i++)
  printf("\x8");
  printf("\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8");*/

  for (i=0 ; i<numtextures ; i++, directory+=4)
  {
    //if (!(i&63))printf (".");

    if (i == numtextures1)
    {
      // Start looking in second texture file.
      maptex = maptex2;
      maxoff = maxoff2;
      directory = maptex+4;
    }

    offset = INTFromBytes((const byte*)directory);

    if (offset > maxoff)
      I_Error ("R_InitTextures: bad texture directory");

    mtexture = (maptexture_t *) ( (byte *)maptex + offset);

    texture = textures[i] = (texture_t*)Z_Malloc (sizeof(texture_t)+ sizeof(texpatch_t)*(SHORT(mtexture->patchcount)-1),PU_STATIC, 0);

    texture->width = SHORT(mtexture->width);
    texture->height = SHORT(mtexture->height);
    texture->patchcount = SHORT(mtexture->patchcount);
    //printf("mtex= %i, tex= %i \n",texture->width,mtexture->height);

    memcpy (texture->name, mtexture->name, sizeof(texture->name));
    mpatch = &mtexture->patches[0];
    patch = &texture->patches[0];

    //printf("%s, patchcount= %i \n",texture->name,texture->patchcount);

    //printf ("texture->name = %s \n",texture->name);
    for (j=0 ; j<texture->patchcount ; j++, mpatch++, patch++)
    {
      patch->originx = SHORT(mpatch->originx);
      patch->originy = SHORT(mpatch->originy);
      patch->patch = patchlookup[SHORT(mpatch->patch)];
      if (patch->patch == -1)
      {
        /*printf ("texture->name= %s \n",texture->name);
        printf ("patchcount= %i \n", texture->patchcount);
        printf ("patch->originx= %i \n",patch->originx);
        printf ("patch->originy= %i \n",patch->originy);
        printf ("patch->patch= %i \n",patch->patch);
        printf ("mpatch->patch= %i \n",mpatch->patch);*/
        I_Error ("R_InitTextures: Missing patch in texture %s", texture->name);
      }
    }		
    texturecolumnlump[i] = (short*)Z_Malloc (texture->width*2, PU_STATIC,0);
    texturecolumnofs[i] = (unsigned short*)Z_Malloc (texture->width*2, PU_STATIC,0);

    j = 1;
    while (j*2 <= texture->width)
      j<<=1;

    texturewidthmask[i] = j-1;
    textureheight[i] = texture->height<<FRACBITS;

    totalwidth += texture->width;
  }

  Z_Free (maptex1);
  if (maptex2)
    Z_Free (maptex2);

  // Precalculate whatever possible.	
  for (i=0 ; i<numtextures ; i++)
    R_GenerateLookup (i);

  // Create translation table for global animation.
  texturetranslation = (int *)Z_Malloc ((numtextures+1)*4, PU_STATIC, 0);

  for (i=0 ; i<numtextures ; i++)
    texturetranslation[i] = i;

  //Nspire has no alloca() function for temporary memory, so we have to use malloc() and free() to compensate
  //CGD: Prizm has malloc / free, but only 128 KB heap
  free(patchlookup);
}



//
// R_InitFlats
//
void R_InitFlats (void)
{
  int		i;

  firstflat = W_GetNumForName ("F_START") + 1;
  lastflat = W_GetNumForName ("F_END") - 1;
  numflats = lastflat - firstflat + 1;

  // Create translation table for global animation.
  flattranslation = (int *)Z_Malloc ((numflats+1)*4, PU_STATIC, 0);

  for (i=0 ; i<numflats ; i++)
    flattranslation[i] = i;
}


//
// R_InitSpriteLumps
// Finds the width and hoffset of all sprites in the wad,
//  so the sprite does not need to be cached completely
//  just for having the header info ready during rendering.
//
void R_InitSpriteLumps (void)
{
  int		i;
  const patch_t	*patch;

  firstspritelump = W_GetNumForName ("S_START") + 1;
  lastspritelump = W_GetNumForName ("S_END") - 1;

  numspritelumps = lastspritelump - firstspritelump + 1;
  spritewidth = (fixed_t *)Z_Malloc (numspritelumps*4, PU_STATIC, 0);
  spriteoffset = (fixed_t *)Z_Malloc (numspritelumps*4, PU_STATIC, 0);
  spritetopoffset = (fixed_t *)Z_Malloc (numspritelumps*4, PU_STATIC, 0);

  for (i=0 ; i< numspritelumps ; i++)
  {
    //if (!(i&63))printf (".");

    patch = W_CacheLumpNumPatch(firstspritelump+i, PU_CACHE);
    spritewidth[i] = SHORT(patch->width)<<FRACBITS;
    spriteoffset[i] = SHORT(patch->leftoffset)<<FRACBITS;
    spritetopoffset[i] = SHORT(patch->topoffset)<<FRACBITS;
  }
}



//
// R_InitColormaps
//
void R_InitColormaps (void)
{
  //int	lump, length;

  // Load in the light tables, 
  //  256 byte align tables.
  //lump = W_GetNumForName("COLORMAP"); 
  /*length = W_LumpLength (lump) + 255; 
  colormaps = (lighttable_t *)Z_Malloc (length, PU_STATIC, 0); 
  colormaps = (lighttable_t *)( ((int)colormaps + 255)&~0xff); 
  W_ReadLump (lump,colormaps); 
*/
  colormaps = (lighttable_t *)W_CacheLumpNameConst ("COLORMAP", PU_STATIC);
}



//
// R_InitData
// Locates all the lumps
//  that will be used by all views
// Must be called after W_Init.
//
void R_InitData (void)
{
  //printf ("InitTextures\n");
  R_InitTextures ();
  //printf ("InitFlats\n");
  R_InitFlats ();
  //printf ("InitSprites\n");
  R_InitSpriteLumps ();
  //printf ("InitColormaps\n");
  R_InitColormaps ();
}



//
// R_FlatNumForName
// Retrieval, get a flat number for a flat name.
//
int R_FlatNumForName (const char* name)
{
  int		i;

  //Note: This copy may seem strange, but we really really really -don't- want to corrupt the value at the original pointer
  //CGD: problem solved: int R_FlatNumForName (char* name) -> int R_FlatNumForName (const char* name)
  i = W_CheckNumForName (name);
  if (i == -1)
  {
    I_Error ("R_FlatNumForName: %s not found",name);
  }

  //printf(", %i \n", i-firstflat);

  return i - firstflat;
}




//
// R_CheckTextureNumForName
// Check whether texture is available.
// Filter out NoTexture indicator.
//
int	R_CheckTextureNumForName (const char *name)
{
  int     i;
  // "NoTexture" marker.
  if(name[0] == '-')
  {
    return 0;
  }

  for(i=0 ; i<numtextures ; i++)
  {
    if(!CGDstrnicmp(textures[i]->name,name,8))
    {
      return i;
    }
  }
  return -1;
}



//
// R_TextureNumForName
// Calls R_CheckTextureNumForName,
//  aborts with error message.
//
int	R_TextureNumForName (const char* name)
{
  int		i;

  i = R_CheckTextureNumForName (name);

  if (i==-1)
  {
    //printf ("Texture Not Found: '%s' \n",name);
    I_Error ("R_TextureNumForName: %s not found", name);
  }
  return i;
}


#if 0

//
// R_PrecacheLevel
// Preloads all relevant graphics for the level.
//
int		flatmemory;
int		texturememory;
int		spritememory;

void R_PrecacheLevel (void)
{
  char*			flatpresent;
  char*			texturepresent;
  char*			spritepresent;

  int				i;
  int				j;
  int				k;
  int				lump;

  texture_t*		texture;
  thinker_t*		th;
  spriteframe_t*	sf;

  // Precache flats.
  //flatpresent = alloca(numflats);
  flatpresent = (char*)CGDMalloc(numflats);
  memset (flatpresent,0,numflats);	

  for (i=0 ; i<numsectors ; i++)
  {
    flatpresent[sectors[i].floorpic] = 1;
    flatpresent[sectors[i].ceilingpic] = 1;
  }

  flatmemory = 0;

  for (i=0 ; i<numflats ; i++)
  {
    if (flatpresent[i])
    {
      lump = firstflat + i;
      flatmemory += lumpinfo[lump].size;
      W_CacheLumpNum(lump, PU_CACHE);
    }
  }

  // Precache textures.
  //texturepresent = alloca(numtextures);
  texturepresent = (char*)CGDMalloc(numtextures);
  memset (texturepresent,0, numtextures);

  for (i=0 ; i<numsides ; i++)
  {
    texturepresent[sides[i].toptexture] = 1;
    texturepresent[sides[i].midtexture] = 1;
    texturepresent[sides[i].bottomtexture] = 1;
  }

  // Sky texture is always present.
  // Note that F_SKY1 is the name used to
  //  indicate a sky floor/ceiling as a flat,
  //  while the sky texture is stored like
  //  a wall texture, with an episode dependend
  //  name.
  texturepresent[skytexture] = 1;

  texturememory = 0;
  for (i=0 ; i<numtextures ; i++)
  {
    if (!texturepresent[i])
      continue;

    texture = textures[i];

    for (j=0 ; j<texture->patchcount ; j++)
    {
      lump = texture->patches[j].patch;
      texturememory += lumpinfo[lump].size;
      W_CacheLumpNum(lump , PU_CACHE);
    }
  }

  // Precache sprites.
  //spritepresent = alloca(numsprites);
  spritepresent = (char*)CGDMalloc (numsprites);
  memset (spritepresent,0, numsprites);

  for (th = thinkercap.next ; th != &thinkercap ; th=th->next)
  {
    if (th->function == TP_MobjThinker)
      spritepresent[((mobj_t *)th)->sprite] = 1;
  }

  spritememory = 0;
  for (i=0 ; i<numsprites ; i++)
  {
    if (!spritepresent[i])
      continue;

    for (j=0 ; j<sprites[i].numframes ; j++)
    {
      sf = &sprites[i].spriteframes[j];
      for (k=0 ; k<8 ; k++)
      {
        lump = firstspritelump + sf->lump[k];
        spritememory += lumpinfo[lump].size;
        W_CacheLumpNum(lump , PU_CACHE);
      }
    }
  }

  //Nspire has no alloca() function for temporary memory, so we have to use malloc() and free() to compensate
  free(flatpresent);
  free(texturepresent);
  free(spritepresent);
}
#endif 