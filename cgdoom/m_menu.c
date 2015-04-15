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
//	DOOM selection menu, options, episode etc.
//	Sliders and icons. Kinda widget stuff.
//
//-----------------------------------------------------------------------------

//static const char 

#include "os.h"


#include "doomdef.h"
#include "dstrings.h"

#include "d_main.h"

#include "i_system.h"
#include "i_video.h"
#include "z_zone.h"
#include "v_video.h"
#include "w_wad.h"

#include "r_local.h"


#include "hu_stuff.h"

#include "g_game.h"

//#include "m_argv.h"
#include "m_swap.h"

#include "doomstat.h"

// Data.

#include "m_menu.h"



extern const patch_t*		hu_font[HU_FONTSIZE];
extern boolean		message_dontfuckwithme;

extern boolean		chat_on;		// in heads-up code

//
// defaulted values
//
int			mouseSensitivity;       // has default

// Show messages has default, 0 = off, 1 = on
int			showMessages= 1;
	

// Blocky mode, has default, 0 = high, 1 = normal
int			detailLevel;		
int			screenblocks;		// has default

// temp for screenblocks (0-9)
int			screenSize;		

// -1 = no quicksave slot picked!
int			quickSaveSlot;          

 // 1 = message to be printed
int			messageToPrint;
// ...and here is the message string!
char*			messageString;		

// message x & y
int			messx;			
int			messy;
int			messageLastMenuActive;

// timed message = no input from user
boolean			messageNeedsInput;     

void    (*messageRoutine)(int response);

#define SAVESTRINGSIZE 	24

/*char gammamsg[5][26] =
{
    GAMMALVL0,
    GAMMALVL1,
    GAMMALVL2,
    GAMMALVL3,
    GAMMALVL4
};*/

// we are going to be entering a savegame string
int			saveStringEnter;              
int             	saveSlot;	// which slot to save in
int			saveCharIndex;	// which char we're editing
// old save description before edit
//char			saveOldString[SAVESTRINGSIZE];  

boolean			inhelpscreens;
boolean			menuactive;

#define SKULLXOFF		-32
#define LINEHEIGHT		16

extern boolean		sendpause;
//char			savegamestrings[10][SAVESTRINGSIZE];

char	endstring[160];


//
// MENU TYPEDEFS
//
typedef struct
{
    // 0 = no cursor here, 1 = ok, 2 = arrows ok
    short	status;
    
    char	name[10];
    
    // choice = menu item #.
    // if status = 2,
    //   choice=0:leftarrow,1:rightarrow
    void	(*routine)(int choice);
    
    // hotkey in menu
    char	alphaKey;			
} menuitem_t;



typedef struct menu_s
{
    short		numitems;	// # of menu items
    struct menu_s*	prevMenu;	// previous menu
    menuitem_t*		menuitems;	// menu items
    void		(*routine)();	// draw routine
    short		x;
    short		y;		// x,y of menu
    short		lastOn;		// last item user was on in menu
} menu_t;

short		itemOn;			// menu item skull is on
short		skullAnimCounter;	// skull animation counter
short		whichSkull;		// which skull to draw

// graphic name of skulls
// warning: initializer-string for array of chars is too long
char    skullName[2][/*8*/9] = {"M_SKULL1","M_SKULL2"};

// current menudef
menu_t*	currentMenu;

//
// PROTOTYPES
//
void M_NewGame(int choice);
void M_Episode(int choice);
void M_ChooseSkill(int choice);
//void M_LoadGame(int choice);
//void M_SaveGame(int choice);
//void M_Options(int choice);
void M_EndGame(int choice);
void M_ReadThis(int choice);
void M_ReadThis2(int choice);
void M_QuitDOOM(int choice);

void M_ChangeMessages(int choice);
//void M_ChangeSensitivity(int choice);
//void M_SfxVol(int choice);
//void M_MusicVol(int choice);
void M_ChangeDetail(int choice);
//void M_SizeDisplay(int choice);
void M_StartGame(int choice);
//void M_Sound(int choice);

void M_FinishReadThis(int choice);
void M_LoadSelect(int choice);
void M_SaveSelect(int choice);
void M_ReadSaveStrings(void);
void M_QuickSave(void);
void M_QuickLoad(void);

void M_DrawMainMenu(void);
void M_DrawReadThis1(void);
void M_DrawReadThis2(void);
void M_DrawNewGame(void);
void M_DrawEpisode(void);
void M_DrawOptions(void);
//void M_DrawSound(void);
//void M_DrawLoad(void);
//void M_DrawSave(void);

void M_DrawSaveLoadBorder(int x,int y);
void M_SetupNextMenu(menu_t *menudef);
void M_DrawThermo(int x,int y,int thermWidth,int thermDot);
void M_DrawEmptyCell(menu_t *menu,int item);
void M_DrawSelCell(menu_t *menu,int item);
void M_WriteText(int x, int y, char *string);
int  M_StringWidth(char *string);
int  M_StringHeight(char *string);
void M_StartControlPanel(void);
void M_StartMessage(char *string,void *routine,boolean input);
void M_StopMessage(void);
void M_ClearMenus (void);




//
// DOOM MENU
//
enum
{
    newgame = 0,
    options,
    loadgame,
    savegame,
    readthis,
    quitdoom,
    main_end
} main_e;

menuitem_t MainMenu[]=
{
    {1,"M_NGAME",M_NewGame,'n'},
//CGD: hack to remove but keep menu size
    {1,"M_OPTION",M_NewGame/*M_Options*/,'o'},
    {1,"M_LOADG",M_NewGame/*M_LoadGame*/,'l'},
    {1,"M_SAVEG",M_NewGame/*M_SaveGame*/,'s'},
    // Another hickup with Special edition.
    {1,"M_RDTHIS",M_ReadThis,'r'},
    {1,"M_QUITG",M_QuitDOOM,'q'}
};

menu_t  MainDef =
{
    main_end,
    NULL,
    MainMenu,
    M_DrawMainMenu,
    97,64,
    0
};


//
// EPISODE SELECT
//
enum
{
    ep1,
    ep2,
    ep3,
    ep4,
    ep_end
} episodes_e;

menuitem_t EpisodeMenu[]=
{
    {1,"M_EPI1", M_Episode,'k'},
    {1,"M_EPI2", M_Episode,'t'},
    {1,"M_EPI3", M_Episode,'i'},
    {1,"M_EPI4", M_Episode,'t'}
};

menu_t  EpiDef =
{
    ep_end,			// # of menu items
    &MainDef,		// previous menu
    EpisodeMenu,	// menuitem_t ->
    M_DrawEpisode,	// drawing routine ->
    48,63,			// x,y
    ep1				// lastOn
};

//
// NEW GAME
//
enum
{
    killthings,
    toorough,
    hurtme,
    violence,
    nightmare,
    newg_end
} newgame_e;

menuitem_t NewGameMenu[]=
{
    {1,"M_JKILL",	M_ChooseSkill, 'i'},
    {1,"M_ROUGH",	M_ChooseSkill, 'h'},
    {1,"M_HURT",	M_ChooseSkill, 'h'},
    {1,"M_ULTRA",	M_ChooseSkill, 'u'},
    {1,"M_NMARE",	M_ChooseSkill, 'n'}
};

menu_t  NewDef =
{
    newg_end,		// # of menu items
    &EpiDef,		// previous menu
    NewGameMenu,	// menuitem_t ->
    M_DrawNewGame,	// drawing routine ->
    48,63,          // x,y
    hurtme			// lastOn
};



//
// OPTIONS MENU
//
enum
{
    endgame,
    messages,
    detail,
    opt_end
} options_e;

menuitem_t OptionsMenu[]=
{
    {1,"M_ENDGAM",	M_EndGame,'e'},
    {1,"M_MESSG",	M_ChangeMessages,'m'},
    {1,"M_DETAIL",	M_ChangeDetail,'g'}
};

menu_t  OptionsDef =
{
    opt_end,
    &MainDef,
    OptionsMenu,
    M_DrawOptions,
    60,37,
    0
};

//
// Read This! MENU 1 & 2
//
enum
{
    rdthsempty1,
    read1_end
} read_e;

menuitem_t ReadMenu1[] =
{
    {1,"",M_ReadThis2,0}
};

menu_t  ReadDef1 =
{
    read1_end,
    &MainDef,
    ReadMenu1,
    M_DrawReadThis1,
    280,185,
    0
};

enum
{
    rdthsempty2,
    read2_end
} read_e2;

menuitem_t ReadMenu2[]=
{
    {1,"",M_FinishReadThis,0}
};

menu_t  ReadDef2 =
{
    read2_end,
    &ReadDef1,
    ReadMenu2,
    M_DrawReadThis2,
    330,175,
    0
};

//
// LOAD GAME MENU
//
enum
{
    load1,
    load2,
    load3,
    load4,
    load5,
    load6,
    load_end
} load_e;
/*
menuitem_t LoadMenu[]=
{
    {1,"", M_LoadSelect,'1'},
    {1,"", M_LoadSelect,'2'},
    {1,"", M_LoadSelect,'3'},
    {1,"", M_LoadSelect,'4'},
    {1,"", M_LoadSelect,'5'},
    {1,"", M_LoadSelect,'6'}
};

menu_t  LoadDef =
{
    load_end,
    &MainDef,
    LoadMenu,
    M_DrawLoad,
    80,54,
    0
};

//
// SAVE GAME MENU
//
menuitem_t SaveMenu[]=
{
    {1,"", M_SaveSelect,'1'},
    {1,"", M_SaveSelect,'2'},
    {1,"", M_SaveSelect,'3'},
    {1,"", M_SaveSelect,'4'},
    {1,"", M_SaveSelect,'5'},
    {1,"", M_SaveSelect,'6'}
};

menu_t  SaveDef =
{
    load_end,
    &MainDef,
    SaveMenu,
    M_DrawSave,
    80,54,
    0
};


//
// M_ReadSaveStrings
//  read the strings from the savegame files
//
/*
void M_ReadSaveStrings(void)
{
    FILE*			handle;
    int				count;
    int				i;
    char			name[256];
	
    for (i = 0;i < load_end;i++)
    {
		sprintf(name,SAVEGAMENAME"%d.dsg",i);
		
		handle = fopen (name, "r");
		if (handle == NULL)
		{
			strcpy(&savegamestrings[i][0],EMPTYSTRING);
			LoadMenu[i].status = 0;
			continue;
		}
		//count = read (handle, &savegamestrings[i], SAVESTRINGSIZE);
		//fread ( dest, size elements, count elements, FILE handle );
		count= fread (&savegamestrings[i], SAVESTRINGSIZE, 1, handle);
		
		fclose (handle);
		LoadMenu[i].status = 1;
    }
}


//
// M_LoadGame & Cie.
//
void M_DrawLoad(void)
{
    int             i;
	
    V_DrawPatchDirect (72,28,0,W_CacheLumpNamePatch("M_LOADG",PU_CACHE));
    for (i = 0;i < load_end; i++)
    {
		M_DrawSaveLoadBorder(LoadDef.x,LoadDef.y+LINEHEIGHT*i);
		M_WriteText(LoadDef.x,LoadDef.y+LINEHEIGHT*i,savegamestrings[i]);
    }
}

*/

//
// Draw border for the savegame description
//
void M_DrawSaveLoadBorder(int x,int y)
{
  int             i;

  V_DrawPatchDirect (x-8,y+7,0,W_CacheLumpNamePatch("M_LSLEFT",PU_CACHE));

  for (i = 0;i < 24;i++)
  {
    V_DrawPatchDirect (x,y+7,0,W_CacheLumpNamePatch("M_LSCNTR",PU_CACHE));
    x += 8;
  }

  V_DrawPatchDirect (x,y+7,0,W_CacheLumpNamePatch("M_LSRGHT",PU_CACHE));
}



//
// Read This Menus
// Had a "quick hack to fix romero bug"
//
void M_DrawReadThis1(void)
{
  inhelpscreens = true;

  switch ( gamemode )
  {
  case commercial:
    V_DrawPatchDirect (0,0,0,W_CacheLumpNamePatch("HELP",PU_CACHE));
    break;
  case shareware:
  case registered:
  case retail:
    V_DrawPatchDirect (0,0,0,W_CacheLumpNamePatch("HELP1",PU_CACHE));
    break;
  default:
    break;
  }

  return;
}



//
// Read This Menus - optional second page.
//
void M_DrawReadThis2(void)
{
  inhelpscreens = true;
  switch ( gamemode )
  {
  case retail:
  case commercial:
    // This hack keeps us from having to change menus.
    V_DrawPatchDirect (0,0,0,W_CacheLumpNamePatch("CREDIT",PU_CACHE));
    break;
  case shareware:
  case registered:
    V_DrawPatchDirect (0,0,0,W_CacheLumpNamePatch("HELP2",PU_CACHE));
    break;
  default:
    break;
  }
  return;
}


//
// Change Sfx & Music volumes
//
/*
void M_Sound(int choice)
{
    M_SetupNextMenu(&SoundDef);
}*/




//
// M_DrawMainMenu
//
void M_DrawMainMenu(void)
{
  V_DrawPatchDirect (94,2,0,W_CacheLumpNamePatch("M_DOOM",PU_CACHE));
}




//
// M_NewGame
//
void M_DrawNewGame(void)
{
  V_DrawPatchDirect (96,14,0,W_CacheLumpNamePatch("M_NEWG",PU_CACHE));
  V_DrawPatchDirect (54,38,0,W_CacheLumpNamePatch("M_SKILL",PU_CACHE));
}

void M_NewGame(int choice)
{
	choice = 0;
    if (netgame)
    {
		M_StartMessage(NEWGAME,NULL,false);
		return;
    }
	
    if ( gamemode == commercial )
		M_SetupNextMenu(&NewDef);
    else
		M_SetupNextMenu(&EpiDef);
}


//
//      M_Episode
//
int     epi;

void M_DrawEpisode(void)
{
  V_DrawPatchDirect (54,38,0,W_CacheLumpNamePatch("M_EPISOD",PU_CACHE));
}

void M_VerifyNightmare(int ch)
{
    if (ch != 'y')
		return;
		
    G_DeferedInitNew((skill_t)nightmare,epi+1,1);
    M_ClearMenus ();
}

void M_ChooseSkill(int choice)
{
    /*if (choice == nightmare)
    {
		M_StartMessage(NIGHTMARE,(void *)M_VerifyNightmare,true);
		return;
    }
	*/
    G_DeferedInitNew((skill_t)choice,epi+1,1);
    M_ClearMenus ();
}

void M_Episode(int choice)
{
    if ( (gamemode == shareware) && choice)
    {
		M_StartMessage(SWSTRING,NULL,false);
		M_SetupNextMenu(&ReadDef1);
    }

    // Yet another hack...
    if ( (gamemode == registered) && (choice > 2))
    {
      //printf( "M_Episode: 4th episode requires UltimateDOOM\n");
      choice = 0;
    }
	 
    epi = choice;
    M_SetupNextMenu(&NewDef);
}



//
// M_Options
//
static const char    detailNames[2][9]	= {"M_GDHIGH","M_GDLOW"};
static const char	msgNames[2][9]		= {"M_MSGOFF","M_MSGON"};


void M_DrawOptions(void)
{
  V_DrawPatchDirect (108,15,0,W_CacheLumpNamePatch("M_OPTTTL",PU_CACHE));

  V_DrawPatchDirect (OptionsDef.x + 175,OptionsDef.y+LINEHEIGHT*detail,0,W_CacheLumpNamePatch(detailNames[detailLevel],PU_CACHE));

  V_DrawPatchDirect (OptionsDef.x + 120,OptionsDef.y+LINEHEIGHT*messages,0,W_CacheLumpNamePatch(msgNames[showMessages],PU_CACHE));
}
/*
void M_Options(int choice)
{
	choice = 0;
    M_SetupNextMenu(&OptionsDef);
}*/



//
//      Toggle messages on/off
//
void M_ChangeMessages(int choice)
{
    // warning: unused parameter `int choice'
    choice = 0;
    showMessages = 1 - showMessages;
	
    if (!showMessages)
	players[consoleplayer].message = MSGOFF;
    else
	players[consoleplayer].message = MSGON ;

    message_dontfuckwithme = true;
}


//
// M_EndGame
//
void M_EndGameResponse(int ch)
{
    if (ch != 'y')
	return;
		
    currentMenu->lastOn = itemOn;
    M_ClearMenus ();
    D_StartTitle ();
}

void M_EndGame(int choice)
{
    choice = 0;
    if (!usergame)
		return;
	
//    M_StartMessage(ENDGAME,(void*)M_EndGameResponse,true);
}




//
// M_ReadThis
//
void M_ReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef1);
}

void M_ReadThis2(int choice)
{
    choice = 0;
    M_SetupNextMenu(&ReadDef2);
}

void M_FinishReadThis(int choice)
{
    choice = 0;
    M_SetupNextMenu(&MainDef);
}

void M_QuitResponse(int ch)
{
    if (ch != 'y')
		return;

    I_Quit ();
}




void M_QuitDOOM(int choice)
{
	choice = 0;
	// We pick index 0 which is language sensitive,
	//  or one at random, between 1 and maximum number.
	//sprintf(endstring,"%s\n\n"DOSY, endmsg[ (gametic%(NUM_QUITMESSAGES-2))+1 ]);

//	M_StartMessage(QUITMSG/*endstring*/,(void *)M_QuitResponse,true);
}




void M_ChangeSensitivity(int choice)
{
    switch(choice)
    {
      case 0:
	if (mouseSensitivity)
	    mouseSensitivity--;
	break;
      case 1:
	if (mouseSensitivity < 9)
	    mouseSensitivity++;
	break;
    }
}




void M_ChangeDetail(int choice)
{
    choice = 0;
    detailLevel = 1 - detailLevel;

    // FIXME - does not work. Remove anyway?
    //printf( "M_ChangeDetail: low detail mode n.a.\n");

    return;
    
    /*R_SetViewSize (screenblocks, detailLevel);

    if (!detailLevel)
	players[consoleplayer].message = DETAILHI;
    else
	players[consoleplayer].message = DETAILLO;*/
}

//
//      Menu Functions
//
void M_DrawThermo ( int	x,  int	y,  int	thermWidth,  int	thermDot )
{
  int xx;
  int i;

  xx = x;
  V_DrawPatchDirect(xx,y,0,W_CacheLumpNamePatch("M_THERML",PU_CACHE));
  xx += 8;
  for (i=0;i<thermWidth;i++)
  {
    V_DrawPatchDirect(xx,y,0,W_CacheLumpNamePatch("M_THERMM",PU_CACHE));
    xx += 8;
  }
  V_DrawPatchDirect(xx,y,0,W_CacheLumpNamePatch("M_THERMR",PU_CACHE));

  V_DrawPatchDirect((x+8) + thermDot*8,y,0,W_CacheLumpNamePatch("M_THERMO",PU_CACHE));
}



void M_DrawEmptyCell( menu_t*	menu,  int		item )
{
  V_DrawPatchDirect (menu->x - 10,menu->y+item*LINEHEIGHT - 1, 0,W_CacheLumpNamePatch("M_CELL1",PU_CACHE));
}

void M_DrawSelCell( menu_t*	menu,  int		item )
{
  V_DrawPatchDirect (menu->x - 10,menu->y+item*LINEHEIGHT - 1, 0,W_CacheLumpNamePatch("M_CELL2",PU_CACHE));
}


void M_StartMessage
( char*		string,
  void*		routine,
  boolean	input )
{
    messageLastMenuActive = menuactive;
    messageToPrint = 1;
    messageString = string;
    messageRoutine = (void (*)(int))routine;
    messageNeedsInput = input;
    menuactive = true;
    return;
}



void M_StopMessage(void)
{
    menuactive = (boolean)messageLastMenuActive;
    messageToPrint = 0;
}



//
// Find string width from hu_font chars
//
int M_StringWidth(char* string)
{
    int             i;
    int             w = 0;
    int             c;
	
    for (i = 0;i < CGDstrlen(string);i++)
    {
	c = toupper_int(string[i]) - HU_FONTSTART;
	if (c < 0 || c >= HU_FONTSIZE)
	    w += 4;
	else
	    w += SHORT (hu_font[c]->width);
    }
		
    return w;
}



//
//      Find string height from hu_font chars
//
int M_StringHeight(char* string)
{
    int             i;
    int             h;
    int             height = SHORT(hu_font[0]->height);
	
    h = height;
    for (i = 0;i < CGDstrlen(string);i++)
	if (string[i] == '\n')
	    h += height;
		
    return h;
}


//
//      Write a string using the hu_font
//
void M_WriteText
( int		x,
  int		y,
  char*		string)
{
    int		w;
    char*	ch;
    int		c;
    int		cx;
    int		cy;
		

    ch = string;
    cx = x;
    cy = y;
	
    for(;;)
    {
	c = *ch++;
	if (!c)
	    break;
	if (c == '\n')
	{
	    cx = x;
	    cy += 12;
	    continue;
	}
		
	c = toupper_int(c) - HU_FONTSTART;
	if (c < 0 || c>= HU_FONTSIZE)
	{
	    cx += 4;
	    continue;
	}
		
	w = SHORT (hu_font[c]->width);
	if (cx+w > SCREENWIDTH)
	    break;
	V_DrawPatchDirect(cx, cy, 0, hu_font[c]);
	cx+=w;
    }
}



//
// CONTROL PANEL
//

//
// M_Responder
//
boolean M_Responder (event_t* ev)
{
    int             ch;
    int             i;
	
    ch = -1;
	
	if (ev->type == ev_keydown)
		ch = ev->data1;
    
    if (ch == -1)
		return false;

    // Take care of any messages that need input
    if (messageToPrint)
    {
	if (messageNeedsInput == true &&
	    !(ch == ' ' || ch == 'n' || ch == 'y' || ch == KEY_ESCAPE))
	    return false;
		
	menuactive = (boolean)messageLastMenuActive;
	messageToPrint = 0;
	if (messageRoutine)
	    messageRoutine(ch);
			
	menuactive = false;
	return true;
    }
	
    // F-Keys
    if (!menuactive)
	switch(ch)
	{
	  case KEY_F1:            // Boss key
	    M_StartControlPanel ();

	    if ( gamemode == retail )
	      currentMenu = &ReadDef2;
	    else
	      currentMenu = &ReadDef1;
	    
	    itemOn = 0;
	    return true;
				
	  case KEY_F2:            // Quicksave
	    //M_QuickSave();
	    return true;
				
	  case KEY_F3:            // Quickload
	    //M_QuickLoad();
	    return true;
	}
    // Pop-up menu?
    if (!menuactive)
    {
		if (ch == KEY_ESCAPE)
		{
			M_StartControlPanel ();
			return true;
		}
			return false;
    }

    
    // Keys usable within menu
    switch (ch)
    {
      case KEY_DOWNARROW:
	do
	{
	    if (itemOn+1 > currentMenu->numitems-1)
		itemOn = 0;
	    else itemOn++;
	} while(currentMenu->menuitems[itemOn].status==-1);
	return true;
		
      case KEY_UPARROW:
	do
	{
	    if (!itemOn)
		itemOn = currentMenu->numitems-1;
	    else itemOn--;
	} while(currentMenu->menuitems[itemOn].status==-1);
	return true;

      case KEY_LEFTARROW:
	if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status == 2)
	{
	    currentMenu->menuitems[itemOn].routine(0);
	}
	return true;
		
      case KEY_RIGHTARROW:
	if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status == 2)
	{
	    currentMenu->menuitems[itemOn].routine(1);
	}
	return true;

      case KEY_ENTER:
	if (currentMenu->menuitems[itemOn].routine && currentMenu->menuitems[itemOn].status)
	{
	    currentMenu->lastOn = itemOn;
	    if (currentMenu->menuitems[itemOn].status == 2)
			currentMenu->menuitems[itemOn].routine(1);      // right arrow
	    else
			currentMenu->menuitems[itemOn].routine(itemOn);
	}
	return true;
		
      case KEY_ESCAPE:
	currentMenu->lastOn = itemOn;
	M_ClearMenus ();
	return true;
		
      case KEY_BACKSPACE:
	currentMenu->lastOn = itemOn;
	if (currentMenu->prevMenu)
	{
	    currentMenu = currentMenu->prevMenu;
	    itemOn = currentMenu->lastOn;
	}
	return true;
	
      default:
	for (i = itemOn+1;i < currentMenu->numitems;i++)
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
			itemOn = (short)i;
			return true;
	    }
	for (i = 0;i <= itemOn;i++)
	    if (currentMenu->menuitems[i].alphaKey == ch)
	    {
			itemOn = (short)i;
            return true;
        }
        break;

    }

    return false;
}



//
// M_StartControlPanel
//
void M_StartControlPanel (void)
{
  // intro might call this repeatedly
  if (menuactive)
    return;

  menuactive = 1;
  currentMenu = &MainDef;         // JDC
  itemOn = currentMenu->lastOn;   // JDC
}


//
// M_Drawer
// Called after the view has been rendered,
// but before it has been blitted.
//
void M_Drawer (void)
{
  static short	x;
  static short	y;
  short			i;
  short			max;
  char			string[40];
  int				start;

  inhelpscreens = false;


  // Horiz. & Vertically center string and print it.
  if (messageToPrint)
  {
    start = 0;
    y = (short)(100 - M_StringHeight(messageString)/2);
    while(*(messageString+start))
    {
      for (i = 0;i < CGDstrlen(messageString+start);i++)
        if (*(messageString+start+i) == '\n')
        {
          memset(string,0,40);
          CGDstrncpy(string,messageString+start,i);
          start += i+1;
          break;
        }

        if (i == CGDstrlen(messageString+start))
        {
          CGDstrcpy(string,messageString+start);
          start += i;
        }

        x = (short)(160 - M_StringWidth(string)/2);
        M_WriteText(x,y,string);
        y = y + SHORT(hu_font[0]->height);
    }
    return;
  }

  if (!menuactive)
    return;

  if (currentMenu->routine)
    currentMenu->routine();         // call Draw routine

  // DRAW MENU
  x = currentMenu->x;
  y = currentMenu->y;
  max = currentMenu->numitems;

  for (i=0;i<max;i++)
  {
    if (currentMenu->menuitems[i].name[0])
      V_DrawPatchDirect (x,y,0,W_CacheLumpNamePatch(currentMenu->menuitems[i].name ,PU_CACHE));
    y += LINEHEIGHT;
  }


  // DRAW SKULL
  V_DrawPatchDirect(x + SKULLXOFF,currentMenu->y - 5 + itemOn*LINEHEIGHT, 0,W_CacheLumpNamePatch(skullName[whichSkull],PU_CACHE));
}


//
// M_ClearMenus
//
void M_ClearMenus (void)
{
    menuactive = 0;
}




//
// M_SetupNextMenu
//
void M_SetupNextMenu(menu_t *menudef)
{
    currentMenu = menudef;
    itemOn = currentMenu->lastOn;
}


//
// M_Ticker
//
void M_Ticker (void)
{
    if (--skullAnimCounter <= 0)
    {
		whichSkull ^= 1;
		skullAnimCounter = 8;
    }
}


//
// M_Init
//
void M_Init (void)
{
    currentMenu = &MainDef;
    menuactive = 0;
    itemOn = currentMenu->lastOn;
    whichSkull = 0;
    skullAnimCounter = 10;
    screenSize = screenblocks - 3;
    messageToPrint = 0;
    messageString = NULL;
    messageLastMenuActive = menuactive;
    quickSaveSlot = -1;

    // Here we could catch other version dependencies,
    //  like HELP1/2, and four episodes.
  
    switch ( gamemode )
    {
		case commercial:
			// This is used because DOOM 2 had only one HELP
			//  page. I use CREDIT as second page now, but
			//  kept this hack for educational purposes.
			MainMenu[readthis] = MainMenu[quitdoom];
			MainDef.numitems--;
			MainDef.y += 8;
			NewDef.prevMenu = &MainDef;
			ReadDef1.routine = M_DrawReadThis1;
			ReadDef1.x = 330;
			ReadDef1.y = 165;
			ReadMenu1[0].routine = M_FinishReadThis;
			break;
		case shareware:
			// Episode 2 and 3 are handled,
			//  branching to an ad screen.
		case registered:
			// We need to remove the fourth episode.
			EpiDef.numitems--;
			break;
		case retail:
			// We are fine.
		default:
			break;
    }
    
}

