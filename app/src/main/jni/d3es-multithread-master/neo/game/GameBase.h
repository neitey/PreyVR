/*
===========================================================================

Doom 3 GPL Source Code
Copyright (C) 1999-2011 id Software LLC, a ZeniMax Media company.

This file is part of the Doom 3 GPL Source Code ("Doom 3 Source Code").

Doom 3 Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#ifndef __GAMEBASE_H__
#define	__GAMEBASE_H__

#define LAGO_IMG_WIDTH			64
#define LAGO_IMG_HEIGHT			64
#define LAGO_WIDTH				64
#define LAGO_HEIGHT				44
#define LAGO_MATERIAL			"textures/sfx/lagometer"
#define LAGO_IMAGE				"textures/sfx/lagometer.tga"

// if set to 1 the server sends the client PVS with snapshots and the client compares against what it sees
#ifndef ASYNC_WRITE_PVS
	#define ASYNC_WRITE_PVS 0
#endif

// the "gameversion" client command will print this plus compile date
#define	GAME_VERSION		"basePrey-1"

#define	MAX_CLIENTS				32
#define	GENTITYNUM_BITS			12
#define	MAX_GENTITIES			(1<<GENTITYNUM_BITS)
//HUMANHEAD rww - client entities!
#define GENTITYNUM_BITS_PLUSCENT	13 //8192 allowable
#define	MAX_CENTITIES				MAX_GENTITIES //it doesn't have to be this, but whatever.
//END HUMANHEAD
#define	ENTITYNUM_NONE			(MAX_GENTITIES-1)
#define	ENTITYNUM_WORLD			(MAX_GENTITIES-2)
#define	ENTITYNUM_MAX_NORMAL	(MAX_GENTITIES-2)

#define MAX_GAME_MESSAGE_SIZE	8192
#define MAX_ENTITY_STATE_SIZE	512
#define ENTITY_PVS_SIZE			((MAX_GENTITIES+31)>>5)

//HUMANHEAD: aob - put here it can be used anywhere.  hhSafeEntity wouldn't compile otherwise.
#if GOLD
// mdl:  Disable if we're gold
#define HH_ASSERT( boolArg )
#else

#ifdef _DEBUG
#define HH_ASSERT( boolArg ) assert( (boolArg) )
#else
#define HH_ASSERT( boolArg ) if( !(boolArg) ) { gameLocal.Error("Assertion Failed: %s, File: %s, Line: %d\n", #boolArg, __FILE__, __LINE__); }
#endif

#endif // GOLD
//HUMANHEAD END


// content masks
#ifdef HUMANHEAD
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID|CONTENTS_FORCEFIELD)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY|CONTENTS_FORCEFIELD|CONTENTS_HUNTERCLIP)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_FORCEFIELD)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_FORCEFIELD|CONTENTS_CORPSE)
#define MASK_SPECTATOR				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_CORPSE)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE) // HUMANHEAD:  Used to be MASK_SOLID for us
#define	MASK_SPIRITPLAYER			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY|CONTENTS_SPIRITBRIDGE)						// HUMANHEAD:  Identical to playersolid, except for forcefield and spiritbridge
#define MASK_VISIBILITY				(CONTENTS_OPAQUE|CONTENTS_RENDERMODEL|CONTENTS_BODY)
#define MASK_TRACTORBEAM			(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_BODY|CONTENTS_FORCEFIELD|CONTENTS_SHOOTABLE|CONTENTS_CORPSE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_SHOOTABLE|CONTENTS_CORPSE|CONTENTS_WATER)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY|CONTENTS_SHOOTABLE|CONTENTS_CORPSE|CONTENTS_WATER)
#define	MASK_SHOTANDPROJECTILES		(MASK_SHOT_RENDERMODEL|CONTENTS_PROJECTILE)														// for shooting projectiles with projectiles
#define MASK_SPIRITARROW			(CONTENTS_SOLID|CONTENTS_RENDERMODEL|CONTENTS_CORPSE|CONTENTS_WATER|CONTENTS_SHOOTABLEBYARROW)	// No forcefield (forcefields/spiritsecrets)
#else	// HUMANHEAD END
#define	MASK_ALL					(-1)
#define	MASK_SOLID					(CONTENTS_SOLID)
#define	MASK_MONSTERSOLID			(CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_BODY)
#define	MASK_PLAYERSOLID			(CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_BODY)
#define	MASK_DEADSOLID				(CONTENTS_SOLID|CONTENTS_PLAYERCLIP)
#define	MASK_WATER					(CONTENTS_WATER)
#define	MASK_OPAQUE					(CONTENTS_OPAQUE)
#define	MASK_SHOT_RENDERMODEL		(CONTENTS_SOLID|CONTENTS_RENDERMODEL)
#define	MASK_SHOT_BOUNDINGBOX		(CONTENTS_SOLID|CONTENTS_BODY)
#endif	// HUMANHEAD

#define DEFAULT_GRAVITY_STRING	"1066"

extern void gameError( const char *fmt, ... );

#endif
