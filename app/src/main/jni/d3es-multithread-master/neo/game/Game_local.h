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

#ifndef __GAME_LOCAL_H__
#define	__GAME_LOCAL_H__

#include "GameBase.h"

#include "idlib/containers/StrList.h"
#include "idlib/containers/LinkList.h"
#include "idlib/BitMsg.h"
#include "Game.h"

#include "gamesys/SaveGame.h"
#include "physics/Clip.h"
#include "physics/Push.h"
#include "script/Script_Program.h"
#include "ai/AAS.h"
#include "anim/Anim.h"
// HUMANHEAD nla
#include "Prey/game_anim.h"
#include "Prey/game_animBlend.h"
#include "Prey/prey_animator.h"
// HUMANHEAD END
#include "Pvs.h"
#include "MultiplayerGame.h"

// HUMANHEAD
#include "Prey/prey_camerainterpolator.h"	// HUMANHEAD
#include "Prey/anim_baseanim.h"				// HUMANHEAD nla - For playing partial animations
#define NUM_AAS 3
// HUMANHEAD END

#include "../../../Doom3Quest/VrClientInfo.h"
#include "framework/async/MsgChannel.h"

#ifdef ID_DEBUG_UNINITIALIZED_MEMORY
// This is real evil but allows the code to inspect arbitrary class variables.
#define private		public
#define protected	public
#endif

/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/
class idDeclEntityDef;
class idRenderWorld;
class idSoundWorld;
class idUserInterface;

// classes used by idGameLocal
class idEntity;
class idActor;
class idPlayer;
class idCamera;
class idWorldspawn;
class idTestModel;
class idSmokeParticles;
class idEntityFx;
class idTypeInfo;
class idThread;
class idEditEntities;
class idLocationEntity;

//============================================================================
extern const int NUM_RENDER_PORTAL_BITS;

void gameError( const char *fmt, ... );

extern idRenderWorld *				gameRenderWorld;
extern idSoundWorld *				gameSoundWorld;

// HUMANHEAD pdm
#if INGAME_PROFILER_ENABLED
extern hhProfiler *					profiler;
#endif
// HUMANHEAD END

extern const int NUM_RENDER_PORTAL_BITS;
/*
===============================================================================

	Local implementation of the public game interface.

===============================================================================
*/
typedef struct entityState_s {
	int						entityNumber;
	idBitMsg				state;
	byte					stateBuf[MAX_ENTITY_STATE_SIZE];
	struct entityState_s *	next;
} entityState_t;

typedef struct snapshot_s {
	int						sequence;
	entityState_t *			firstEntityState;
	int						pvs[ENTITY_PVS_SIZE];
	struct snapshot_s *		next;
} snapshot_t;

struct timeState_t {
	int					time;
	int					previousTime;
	int					msec;
	int					framenum;
	int					realClientTime;

	void				Set(int t, int pt, int ms, int f, int rct)		{
		time = t;
		previousTime = pt;
		msec = ms;
		framenum = f;
		realClientTime = rct;
	};
	void				Get(int &t, int &pt, int &ms, int &f, int &rct)	{
		t = time;
		pt = previousTime;
		ms = msec;
		f = framenum;
		rct = realClientTime;
	};
	void				Save(idSaveGame *savefile) const	{
		savefile->WriteInt(time);
		savefile->WriteInt(previousTime);
		savefile->WriteInt(msec);
		savefile->WriteInt(framenum);
		savefile->WriteInt(realClientTime);
	}
	void				Restore(idRestoreGame *savefile)	{
		savefile->ReadInt(time);
		savefile->ReadInt(previousTime);
		savefile->ReadInt(msec);
		savefile->ReadInt(framenum);
		savefile->ReadInt(realClientTime);
	}
	void				Increment()							{
		framenum++;
		previousTime = time;
		time += msec;
		realClientTime = time;
	};
};

enum slowmoState_t {
	SLOWMO_STATE_OFF,
	SLOWMO_STATE_RAMPUP,
	SLOWMO_STATE_ON,
	SLOWMO_STATE_RAMPDOWN
};

const int MAX_EVENT_PARAM_SIZE		= 128;

//HUMANHEAD rww - for assistance in cleaning up garbage events for ents that no longer exist on client
//#define _HH_NET_EVENT_TYPE_VALIDATION
//HUMANHEAD END

typedef struct entityNetEvent_s {
	int						spawnId;
	int						event;
	int						time;
	int						paramsSize;
	byte					paramsBuf[MAX_EVENT_PARAM_SIZE];
#ifdef _HH_NET_EVENT_TYPE_VALIDATION //HUMANHEAD rww
	int						entTypeId;
#endif //HUMANHEAD END
	struct entityNetEvent_s	*next;
	struct entityNetEvent_s *prev;
} entityNetEvent_t;

enum {
	GAME_RELIABLE_MESSAGE_INIT_DECL_REMAP,
	GAME_RELIABLE_MESSAGE_REMAP_DECL,
	GAME_RELIABLE_MESSAGE_SPAWN_PLAYER,
	GAME_RELIABLE_MESSAGE_DELETE_ENT,
	GAME_RELIABLE_MESSAGE_CHAT,
	//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
	GAME_RELIABLE_MESSAGE_SPECIAL,
	//HUMANHEAD END
	GAME_RELIABLE_MESSAGE_TCHAT,
	GAME_RELIABLE_MESSAGE_SOUND_EVENT,
	GAME_RELIABLE_MESSAGE_SOUND_INDEX,
	GAME_RELIABLE_MESSAGE_DB,
	GAME_RELIABLE_MESSAGE_DB_DEATH, //HUMANHEAD rww
	GAME_RELIABLE_MESSAGE_KILL,
	GAME_RELIABLE_MESSAGE_DROPWEAPON,
	GAME_RELIABLE_MESSAGE_RESTART,
	GAME_RELIABLE_MESSAGE_SERVERINFO,
	GAME_RELIABLE_MESSAGE_TOURNEYLINE,
	GAME_RELIABLE_MESSAGE_CALLVOTE,
	GAME_RELIABLE_MESSAGE_CASTVOTE,
	GAME_RELIABLE_MESSAGE_STARTVOTE,
	GAME_RELIABLE_MESSAGE_UPDATEVOTE,
	GAME_RELIABLE_MESSAGE_PORTALSTATES,
	GAME_RELIABLE_MESSAGE_PORTAL,
	GAME_RELIABLE_MESSAGE_VCHAT,
	GAME_RELIABLE_MESSAGE_STARTSTATE,
	GAME_RELIABLE_MESSAGE_MENU,
	GAME_RELIABLE_MESSAGE_WARMUPTIME,
	GAME_RELIABLE_MESSAGE_EVENT
};

typedef enum {
	GAMESTATE_UNINITIALIZED,		// prior to Init being called
	GAMESTATE_NOMAP,				// no map loaded
	GAMESTATE_STARTUP,				// inside InitFromNewMap().  spawning map entities.
	GAMESTATE_ACTIVE,				// normal gameplay
	GAMESTATE_SHUTDOWN				// inside MapShutdown().  clearing memory.
} gameState_t;

typedef struct {
	idEntity	*ent;
	int			dist;
	int			team;
} spawnSpot_t;

//============================================================================

class idEventQueue {
public:
	typedef enum {
		OUTOFORDER_IGNORE,
		OUTOFORDER_DROP,
		OUTOFORDER_SORT
	} outOfOrderBehaviour_t;

							idEventQueue() : start( NULL ), end( NULL ) {}

	entityNetEvent_t *		Alloc();
	void					Free( entityNetEvent_t *event );
	void					Shutdown();

	void					Init();
	void					Enqueue( entityNetEvent_t* event, outOfOrderBehaviour_t oooBehaviour );
	entityNetEvent_t *		Dequeue( void );
	entityNetEvent_t *		RemoveLast( void );

	entityNetEvent_t *		Start( void ) { return start; }

private:
	entityNetEvent_t *					start;
	entityNetEvent_t *					end;
	idBlockAlloc<entityNetEvent_t,32>	eventAllocator;
};

//============================================================================

template< class type >
class idEntityPtr {
public:
							idEntityPtr();
	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

#ifndef HUMANHEAD //HUMANHEAD: aob - changed prototype.  new version in HH section
	idEntityPtr<type> &		operator=( type *ent );
#endif

	// synchronize entity pointers over the network
	int						GetSpawnId( void ) const { return spawnId; }
	bool					SetSpawnId( int id );
	bool					UpdateSpawnId( void );

	bool					IsValid( void ) const;
	type *					GetEntity( void ) const;
	int						GetEntityNum( void ) const;

#ifdef HUMANHEAD //HUMANHEAD: aob
	idEntityPtr( const type* ent ) { Assign( ent ); }

	void					Clear();
	idEntityPtr<type> &		Assign( const idEntity *ent );
	idEntityPtr<type> &		Assign( const idEntityPtr<type> &ent );
	idEntityPtr<type> &		operator=( const idEntity *ent );
	idEntityPtr<type> &		operator=( const idEntityPtr<type> &ent );
	type *					operator->() const { return GetEntity(); }
	bool					IsEqualTo( const idEntity *ent ) const;
	bool					IsEqualTo( const idEntityPtr<type> &ent ) const;
	bool					operator==( const idEntity *ent ) const;
	bool					operator==( const idEntityPtr<type> &ent ) const;
	bool					operator!=( const idEntity *ent ) const;
	bool					operator!=( const idEntityPtr<type> &ent ) const;
#endif

private:
	int						spawnId;
};

//============================================================================

class idGameLocal : public idGame {
public:
	idDict					serverInfo;				// all the tunable parameters, like numclients, etc
	int						numClients;				// pulled from serverInfo and verified
	idDict					userInfo[MAX_CLIENTS];	// client specific settings
	usercmd_t				usercmds[MAX_CLIENTS];	// client input commands
	idDict					persistentPlayerInfo[MAX_CLIENTS];
	//HUMANHEAD rww - introducing client entities.
	/*
	idEntity *				entities[MAX_GENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES];// for use in idEntityPtr
	*/
	idEntity *				entities[MAX_GENTITIES+MAX_CENTITIES];// index to entities
	int						spawnIds[MAX_GENTITIES+MAX_CENTITIES];// for use in idEntityPtr
	//END HUMANHEAD
	int						firstFreeIndex;			// first free index in the entities array
	int						num_entities;			// current number <= MAX_GENTITIES
	idHashIndex				entityHash;				// hash table to quickly find entities by name
	idWorldspawn *			world;					// world entity
	idLinkList<idEntity>	spawnedEntities;		// all spawned entities
	idLinkList<idEntity>	activeEntities;			// all thinking entities (idEntity::thinkFlags != 0)
	int						numEntitiesToDeactivate;// number of entities that became inactive in current frame
	bool					sortPushers;			// true if active lists needs to be reordered to place pushers at the front
	bool					sortTeamMasters;		// true if active lists needs to be reordered to place physics team masters before their slaves
	//HUMANHEAD rww
	bool					sortSnapshotPushers;	// true if snapshot lists needs to be reordered to place pushers at the front
	bool					sortSnapshotTeamMasters;// true if snapshot lists needs to be reordered to place physics team masters before their slaves
	//HUMANHEAD END
	idDict					persistentLevelInfo;	// contains args that are kept around between levels

	// can be used to automatically effect every material in the world that references globalParms
	float					globalShaderParms[ MAX_GLOBAL_SHADER_PARMS ];

	idRandom				random;					// random number generator used throughout the game

	idProgram				program;				// currently loaded script and data space
	idThread *				frameCommandThread;

	idClip					clip;					// collision detection
	idPush					push;					// geometric pushing
	idPVS					pvs;					// potential visible set

	idTestModel *			testmodel;				// for development testing of models
	idEntityFx *			testFx;					// for development testing of fx

	idStr					sessionCommand;			// a target_sessionCommand can set this to return something to the session

	idMultiplayerGame		mpGame;					// handles rules for standard dm

	idSmokeParticles *		smokeParticles;			// global smoke trails
	idEditEntities *		editEntities;			// in game editing

	int						cinematicSkipTime;		// don't allow skipping cinemetics until this time has passed so player doesn't skip out accidently from a firefight
	int						cinematicStopTime;		// cinematics have several camera changes, so keep track of when we stop them so that we don't reset cinematicSkipTime unnecessarily
	int						cinematicMaxSkipTime;	// time to end cinematic when skipping.  there's a possibility of an infinite loop if the map isn't set up right.
	bool					inCinematic;			// game is playing cinematic (player controls frozen)
	bool					skipCinematic;

													// are kept up to date with changes to serverInfo
	int						framenum;
	int						previousTime;			// time in msec of last frame
	int						time;					// in msec
	int						msec;					// time since last update in milliseconds

	int						vacuumAreaNum;			// -1 if level doesn't have any outside areas

	gameType_t				gameType;
	bool					isMultiplayer;			// set if the game is run in multiplayer mode
	bool					isServer;				// set if the game is run for a dedicated or listen server
	bool					isClient;				// set if the game is run for a client
													// discriminates between the RunFrame path and the ClientPrediction path
													// NOTE: on a listen server, isClient is false
	int						localClientNum;			// number of the local client. MP: -1 on a dedicated
	idLinkList<idEntity>	snapshotEntities;		// entities from the last snapshot
	int						realClientTime;			// real client time
	bool					isNewFrame;				// true if this is a new game frame, not a rerun due to prediction
	float					clientSmoothing;		// smoothing of other clients in the view
	int						entityDefBits;			// bits required to store an entity def number

	static const char *		sufaceTypeNames[ MAX_SURFACE_TYPES ];	// text names for surface types

	idEntityPtr<idEntity>	lastGUIEnt;				// last entity with a GUI, used by Cmd_NextGUI_f
	int						lastGUI;				// last GUI on the lastGUIEnt

	int						timeRandom;				//HUMANHEAD rww - for time seeding

	idEntityPtr<idEntity>	portalSkyEnt;
	bool					portalSkyActive;

	void					SetPortalSkyEnt(idEntity *ent);
	bool					IsPortalSkyAcive();

	timeState_t				fast;
	timeState_t				slow;

	slowmoState_t			slowmoState;
	float					slowmoMsec;

	bool					quickSlowmoReset;

	virtual void			SelectTimeGroup(int timeGroup);
	virtual int				GetTimeGroupTime(int timeGroup);

	virtual void			GetBestGameType(const char *map, const char *gametype, char buf[ MAX_STRING_CHARS ]);

	void					ComputeSlowMsec();
	void					RunTimeGroup2();

	void					ResetSlowTimeVars();
	void					QuickSlowmoReset();

	bool					NeedRestart();

	// HUMANHEAD
	void					SetSteamTime( int _time ) { nextSteamTime = _time; };
	int						GetSteamTime() { return nextSteamTime; };
	void					SpiritWalkSoundMode(bool active);
	void					DialogSoundMode(bool active);
	// HUMANHEAD END

	//HUMANHEAD rww
	bool					EntInClientSnapshot(int entNum) { return (!isClient || (clientPVS[localClientNum][entNum >> 5] & ( 1 << ( entNum & 31 ) ))); }
	int						GetMapSpawnCount(void) { return mapSpawnCount; }

	void					GetAPUserInfo(idDict &dict, int clientNum);
	void					SpawnArtificialPlayer(void);
	//HUMANHEAD END

	//HUMANHEAD rww - keep track of layered spawning
	//if an object spawns another object in its spawn on mapload, this will completely destroy everything because
	//we need map-load-time entities to be in sync on client and server, and obviously the client cannot spawn them.
	//if this is really required and a postevent spawn cannot be done, then the spawnCount must be incremented to fake
	//the spawn on the client and the server should force the artificial entity to spawn above the map entity count.
	//(this does work as i have tested it, but it's an ugly hack that i would like to avoid)
#if !GOLD
	int						layeredSpawn;
	bool					spawnPopulating;
#endif
	//HUMANHEAD END

	//HUMANHEAD rww
	bool					logitechLCDEnabled;
	bool					logitechLCDDisplayAlt;
	DWORD					logitechLCDButtonsLast;
	int						logitechLCDUpdateTime;
	//HUMANHEAD END

	// ---------------------- Public idGame Interface -------------------

							idGameLocal();

	virtual void			Init( void );
	virtual void			Shutdown( void );
	virtual void			SetLocalClient( int clientNum );
	virtual void 			SetVRClientInfo(vrClientInfo *pVRClientInfo);
	virtual void			EvaluateVRMoveMode(idVec3 &viewangles, usercmd_t &cmd, int buttonCurrentlyClicked, float snapTurn);

	virtual void			CheckRenderCvars();

	//GB Trying to move animator function
	virtual bool			AnimatorGetJointTransform(idAnimator* animator, jointHandle_t jointHandle, int currentTime, idVec3 &offset, idMat3 &axis );
    virtual bool            CMDButtonsAttackCall(int &teleportCanceled);
    virtual bool            CMDButtonsPhysicalCrouch();
	virtual float			CalcTorsoYawDelta(usercmd_t &cmd);
	virtual void			ThrottleUserInfo( void );
	virtual const idDict *	SetUserInfo( int clientNum, const idDict &userInfo, bool isClient, bool canModify );
	virtual const idDict *	GetUserInfo( int clientNum );
	virtual void			SetServerInfo( const idDict &serverInfo );
	virtual bool 			InCinematic();

	virtual const idDict &	GetPersistentPlayerInfo( int clientNum );
	virtual void			SetPersistentPlayerInfo( int clientNum, const idDict &playerInfo );
	virtual void			InitFromNewMap( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, bool isServer, bool isClient, int randSeed );
	virtual bool			InitFromSaveGame( const char *mapName, idRenderWorld *renderWorld, idSoundWorld *soundWorld, idFile *saveGameFile );
	virtual void			SaveGame( idFile *saveGameFile );
	virtual void			MapShutdown( void );
	virtual void			CacheDictionaryMedia( const idDict *dict );
	virtual void			SpawnPlayer( int clientNum );
	virtual gameReturn_t	RunFrame( const usercmd_t *clientCmds );
	virtual void 			EndFrame();
	virtual bool			Draw( int clientNum );
	virtual escReply_t		HandleESC( idUserInterface **gui );
	virtual idUserInterface	*StartMenu( void );
	virtual const char *	HandleGuiCommands( const char *menuCommand );
	virtual void			HandleMainMenuCommands( const char *menuCommand, idUserInterface *gui );
	virtual allowReply_t	ServerAllowClient( int numClients, const char *IP, const char *guid, const char *password, char reason[MAX_STRING_CHARS] );
	virtual void			ServerClientConnect( int clientNum, const char *guid );
	virtual void			ServerClientBegin( int clientNum );
	virtual void			ServerClientDisconnect( int clientNum );
	virtual void			ServerWriteInitialReliableMessages( int clientNum );
	virtual void			ServerWriteSnapshot( int clientNum, int sequence, idBitMsg &msg, byte *clientInPVS, int numPVSClients );
	virtual bool			ServerApplySnapshot( int clientNum, int sequence );
	virtual void			ServerProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual void			ClientReadSnapshot( int clientNum, int sequence, const int gameFrame, const int gameTime, const int dupeUsercmds, const int aheadOfServer, const idBitMsg &msg );
	virtual bool			ClientApplySnapshot( int clientNum, int sequence );
	virtual void			ClientProcessReliableMessage( int clientNum, const idBitMsg &msg );
	virtual gameReturn_t	ClientPrediction( int clientNum, const usercmd_t *clientCmds, bool lastPredictFrame );

	//HUMANHEAD rww
	virtual void			LogitechLCDUpdate(void) {}

	virtual void			ServerAddUnreliableSnapMessage(int clientNum, const idBitMsg &msg);
	virtual void			ClientReadUnreliableSnapMessages(int clientNum, const idBitMsg &msg);
	//HUMANHEAD END

	virtual void			GetClientStats( int clientNum, char *data, const int len );

	virtual bool			IsInGame() const
	{
		return GameState() == GAMESTATE_ACTIVE;
	}

	virtual void			SwitchTeam( int clientNum, int team );

	virtual bool			DownloadRequest( const char *IP, const char *guid, const char *paks, char urls[ MAX_STRING_CHARS ] );
	virtual bool			InGameGuiActive();
	virtual bool			ObjectiveSystemActive();
	// ---------------------- Public idGameLocal Interface -------------------

	void					Printf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DPrintf( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Warning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					DWarning( const char *fmt, ... ) const id_attribute((format(printf,2,3)));
	void					Error( const char *fmt, ... ) const id_attribute((format(printf,2,3)));

							// Initializes all map variables common to both save games and spawned games
	void					LoadMap( const char *mapName, int randseed );

	void					LocalMapRestart( void );
	void					MapRestart( void );
	static void				MapRestart_f( const idCmdArgs &args );
	bool					NextMap( void );	// returns wether serverinfo settings have been modified
	static void				NextMap_f( const idCmdArgs &args );

	idMapFile *				GetLevelMap( void );
	const char *			GetMapName( void ) const;

	int						NumAAS( void ) const;
	idAAS *					GetAAS( int num ) const;
	idAAS *					GetAAS( const char *name ) const;
	void					SetAASAreaState( const idBounds &bounds, const int areaContents, bool closed );
	aasHandle_t				AddAASObstacle( const idBounds &bounds );
	void					RemoveAASObstacle( const aasHandle_t handle );
	void					RemoveAllAASObstacles( void );

	bool					CheatsOk( bool requirePlayer = true );
//	void					SetSkill( int value );	// HUMANHEAD pdm: not used
	gameState_t				GameState( void ) const;
	idEntity *				SpawnEntityType( const idTypeInfo &classdef, const idDict *args = NULL, bool bIsClientReadSnapshot = false );
	//HUMANHEAD rww
	idEntity *				SpawnEntityTypeClient( const idTypeInfo &classdef, const idDict *args );
	//HUMANHEAD END

	//HUMANHEAD rww - added clientEntity, bIsClientReadSnapshot
	bool					SpawnEntityDef( const idDict &args, idEntity **ent = NULL, bool setDefaults = true, bool clientEntity = false, bool bIsClientReadSnapshot = false );
	//HUMANHEAD END
	int						GetSpawnId( const idEntity *ent ) const;

	const idDeclEntityDef *	FindEntityDef( const char *name, bool makeDefault = true ) const;
	const idDict *			FindEntityDefDict( const char *name, bool makeDefault = true ) const;

	void					RegisterEntity( idEntity *ent );
	void					UnregisterEntity( idEntity *ent );

	bool					RequirementMet( idEntity *activator, const idStr &requires, int removeItem );

	virtual					//HUMANHEAD jsh made virtual
	void					AlertAI( idEntity *ent );
	idActor *				GetAlertEntity( void );

	bool					InPlayerPVS( idEntity *ent ) const;
	bool					InPlayerConnectedArea( idEntity *ent ) const;
	pvsHandle_t				GetPlayerPVS()			{
		return playerPVS;
	};

	void					SetCamera( idCamera *cam );
	idCamera *				GetCamera( void ) const;
	bool					SkipCinematic( void );
	void					CalcFov( float base_fov, float &fov_x, float &fov_y ) const;

	void					AddEntityToHash( const char *name, idEntity *ent );
	bool					RemoveEntityFromHash( const char *name, idEntity *ent );
	int						GetTargets( const idDict &args, idList< idEntityPtr<idEntity> > &list, const char *ref ) const;

							// returns the master entity of a trace.  for example, if the trace entity is the player's head, it will return the player.
	idEntity *				GetTraceEntity( const trace_t &trace ) const;

	static void				ArgCompletion_EntityName( const idCmdArgs &args, void(*callback)( const char *s ) );
	idEntity *				FindTraceEntity( idVec3 start, idVec3 end, const idTypeInfo &c, const idEntity *skip ) const;
	idEntity *				FindEntity( const char *name ) const;
	idEntity *				FindEntityUsingDef( idEntity *from, const char *match ) const;
	int						EntitiesWithinRadius( const idVec3 org, float radius, idEntity **entityList, int maxCount ) const;
	// HUMANHEAD pdm
	static void				ArgCompletion_ClassName( const idCmdArgs &args, void(*callback)( const char *s ) );
	static void				ArgCompletion_EntityOrClassName( const idCmdArgs &args, void(*callback)( const char *s ) );
	virtual bool			PlayerIsDeathwalking( void );
	virtual unsigned int	GetTimePlayed( void );
	virtual void			ClearTimePlayed( void );

	void					KillBox( idEntity *ent, bool catch_teleport = false );
	//HUMANHEAD PCF rww 05/15/06 - use a custom clipmask killbox (seperated due to pcf paranoia)
	void					KillBoxMasked( idEntity *ent, int clipMask, bool catch_teleport = false );
	//HUMANHEAD END

	virtual					// HUMANHEAD JRM - made virtual
	void					RadiusDamage( const idVec3 &origin, idEntity *inflictor, idEntity *attacker, idEntity *ignoreDamage, idEntity *ignorePush, const char *damageDefName, float dmgPower = 1.0f );
	virtual					// HUMANHEAD JRM - made virtual
	void					RadiusPush( const idVec3 &origin, const float radius, const float push, const idEntity *inflictor, const idEntity *ignore, float inflictorScale, const bool quake );
	void					RadiusPushClipModel( const idVec3 &origin, const float push, const idClipModel *clipModel );

	void					ProjectDecal( const idVec3 &origin, const idVec3 &dir, float depth, bool parallel, float size, const char *material, float angle = 0 );
	void					BloodSplat( const idVec3 &origin, const idVec3 &dir, float size, const char *material );

	void					CallFrameCommand( idEntity *ent, const function_t *frameCommand );
	void					CallObjectFrameCommand( idEntity *ent, const char *frameCommand );

	const idVec3 &			GetGravity( void ) const;

	// added the following to assist licensees with merge issues
	int						GetFrameNum() const { return framenum; };
	int						GetTime() const { return time; };
	int						GetMSec() const { return USERCMD_MSEC; };

	int						GetNextClientNum( int current ) const;
	idPlayer *				GetClientByNum( int current ) const;
	idPlayer *				GetClientByName( const char *name ) const;
	idPlayer *				GetClientByCmdArgs( const idCmdArgs &args ) const;

	idPlayer *				GetLocalPlayer() const;
	// HUMANHEAD nla
	bool 					AmLocalClient( idPlayer *player );
	bool					isCoop;
	bool					IsCooperative() const	{	return isCoop;	}
	bool					IsCompetitive() const	{	return isMultiplayer && !IsCooperative();	}

	// For saving objects that we don't want to have to spawn -mdl
	void					RegisterUniqueObject( idClass *object ); // Meant to be called once by constructor of object being registered
	void					UnregisterUniqueObject( idClass *object ); // Meant to be called by once destructor of object being registered

	virtual void			FocusGUICleanup(idUserInterface *gui); //HUMANHEAD rww

	// HUMANHEAD pdm: print game side memory statistics
	virtual void			PrintMemInfo( MemInfo_t *mi );

// HUMANHEAD pdm: Support for level appending
	// jmarshall - changed to false.
	bool					DeathwalkMapLoaded() const	{	return false;	} // return bShouldAppend; jmarshall
#if DEATHWALK_AUTOLOAD
	bool					bShouldAppend;			// Taken from serverinfo
protected:
	idMapFile *				additionalMapFile;
	virtual void			SpawnAppendedMapEntities() {}
#endif
// HUMANHEAD END

// HUMANHEAD mdl
protected:
	// Special case, these must be virtual.  Allows hhGameLocal to save and restore it's data -mdl
	virtual void			Save( idSaveGame *savefile ) const { }
	virtual void			Restore( idRestoreGame *savefile ) { }

	// For saving objects that we don't want to have to spawn -mdl
	idList<idClass *>		uniqueObjects; // The object list
	idList<int>				uniqueObjRefs; // The reference count list

public:
// HUMANHEAD END


	void					SpreadLocations();
	idLocationEntity *		LocationForPoint( const idVec3 &point );	// May return NULL
	idEntity *				SelectInitialSpawnPoint( idPlayer *player );

	void					SetPortalState( qhandle_t portal, int blockingBits );
	void					SaveEntityNetworkEvent( const idEntity *ent, int event, const idBitMsg *msg );
	void					ServerSendChatMessage( int to, const char *name, const char *text );
	//HUMANHEAD PCF rww 05/10/06 - "fix" for server-localized join messages (this is dumb).
	typedef enum {
		SPECIALMSG_UNKNOWN = 0,
		SPECIALMSG_JOINED,
		//HUMANHEAD PCF rww 05/17/06 - localized vote checks (nested pcf's, hot!)
		SPECIALMSG_ALREADYRUNNINGMAP,
		SPECIALMSG_JUSTONE,
		//HUMANHEAD END
		SPECIALMSG_NUM
	} serverSpecialMsg_e;
	void					ServerSendSpecialMessage( serverSpecialMsg_e msgType, int to, const char *fromNonLoc, int numTextPtrs, const char **text );
	//HUMANHEAD END
	int						ServerRemapDecl( int clientNum, declType_t type, int index );
	int						ClientRemapDecl( declType_t type, int index );

	void					SetGlobalMaterial( const idMaterial *mat );
	const idMaterial *		GetGlobalMaterial();

	virtual bool				IsPDAOpen() const;

	void					SetGibTime( int _time ) { nextGibTime = _time; };
	int						GetGibTime() { return nextGibTime; };

	// Koz made public
	void					SetScriptFPS( const float com_engineHz );
	// Koz end

protected:	// HUMANHEAD
	const static int		INITIAL_SPAWN_COUNT = 1;

	vrClientInfo 		*pVRClientInfo;

	idStr					mapFileName;			// name of the map, empty string if no map loaded
	idMapFile *				mapFile;				// will be NULL during the game unless in-game editing is used
	bool					mapCycleLoaded;

	//HUMANHEAD rww
	class hhDDAManager*		ddaManager;
	//HUMANHEAD END

	//HUMANHEAD jsh
	class hhReactionHandler*	reactionHandler;
	//HUMANHEAD END

	int						spawnCount;
	int						mapSpawnCount;			// it's handy to know which entities are part of the map

	idLocationEntity **		locationEntities;		// for location names, etc

	idCamera *				camera;
	const idMaterial *		globalMaterial;			// for overriding everything

	idList<idAAS *>			aasList;				// area system
	idStrList				aasNames;

	idEntityPtr<idActor>	lastAIAlertEntity;
	int						lastAIAlertTime;

	idDict					spawnArgs;				// spawn args used during entity spawning  FIXME: shouldn't be necessary anymore

	pvsHandle_t				playerPVS;				// merged pvs of all players
	pvsHandle_t				playerConnectedAreas;	// all areas connected to any player area

	idVec3					gravity;				// global gravity vector
	gameState_t				gamestate;				// keeps track of whether we're spawning, shutting down, or normal gameplay
	bool					influenceActive;		// true when a phantasm is happening
	int						nextGibTime;
	int						nextSteamTime;			// HUMANHEAD mdl:  Don't spawn too many steam entities at once

	idList<int>				clientDeclRemap[MAX_CLIENTS][DECL_MAX_TYPES];

	entityState_t *			clientEntityStates[MAX_CLIENTS][MAX_GENTITIES];
	int						clientPVS[MAX_CLIENTS][ENTITY_PVS_SIZE];
	snapshot_t *			clientSnapshots[MAX_CLIENTS];
	idBlockAlloc<entityState_t,256>entityStateAllocator;
	idBlockAlloc<snapshot_t,64>snapshotAllocator;

	idMsgQueue				unreliableSnapMsg[MAX_CLIENTS]; //HUMANHEAD rww - unreliable messages get appended to the snapshot message (since snapshots are unreliable)

	idEventQueue			eventQueue;
	idEventQueue			savedEventQueue;

	idStaticList<spawnSpot_t, MAX_GENTITIES> spawnSpots;
	idStaticList<idEntity *, MAX_GENTITIES> initialSpots;
	int						currentInitialSpot;

	idStaticList<spawnSpot_t, MAX_GENTITIES> teamSpawnSpots[2];
	idStaticList<idEntity *, MAX_GENTITIES> teamInitialSpots[2];
	int						teamCurrentInitialSpot[2];

	idDict					newInfo;

	idStrList				shakeSounds;

	byte					lagometer[ LAGO_IMG_HEIGHT ][ LAGO_IMG_WIDTH ][ 4 ];

	// HUMANHEAD mdl:  Play time
	unsigned int			playTime; // Total play time, not including current session
	unsigned int			playTimeStart; // -1 if not playing, otherwise time when current sesion started
	// HUMANHEAD END

	void					Clear( void );
							// returns true if the entity shouldn't be spawned at all in this game type or difficulty level
	virtual					// HUMANHEAD mdl: Made virtual
	bool					InhibitEntitySpawn( idDict &spawnArgs );
							// spawn entities from the map file
	virtual	// HUMANHEAD JRM - made virtual
	void					SpawnMapEntities( void );
							// commons used by init, shutdown, and restart
	void					MapPopulate( void );
	void					MapClear( bool clearClients );

	pvsHandle_t				GetClientPVS( idPlayer *player, pvsType_t type );
	void					SetupPlayerPVS( void );
	void					FreePlayerPVS( void );
	void					UpdateGravity( void );
	void					SortActiveEntityList( void );
	//HUMANHEAD
	void					SortSnapshotEntityList( void );
	void					RegisterLocationsWithSoundWorld();
	//HUMANHEAD END
	void					ShowTargets( void );
	void					RunDebugInfo( void );

	void					InitScriptForMap( void );

	void					InitConsoleCommands( void );
	void					ShutdownConsoleCommands( void );

	void					InitAsyncNetwork( void );
	void					ShutdownAsyncNetwork( void );
	void					InitLocalClient( int clientNum );
	void					InitClientDeclRemap( int clientNum );
	void					ServerSendDeclRemapToClient( int clientNum, declType_t type, int index );
	void					FreeSnapshotsOlderThanSequence( int clientNum, int sequence );
	bool					ApplySnapshot( int clientNum, int sequence );
	void					WriteGameStateToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadGameStateFromSnapshot( const idBitMsgDelta &msg );
	void					NetworkEventWarning( const entityNetEvent_t *event, const char *fmt, ... ) id_attribute((format(printf,3,4)));
	void					ServerProcessEntityNetworkEventQueue( void );
	void					ClientProcessEntityNetworkEventQueue( void );
	void					ClientShowSnapshot( int clientNum ) const;
							// call after any change to serverInfo. Will update various quick-access flags
	void					UpdateServerInfoFlags( void );
	void					RandomizeInitialSpawns( void );
	static int				sortSpawnPoints( const void *ptr1, const void *ptr2 );

	void					DumpOggSounds( void );
	void					GetShakeSounds( const idDict *dict );

	void					Tokenize( idStrList &out, const char *in );

	void					UpdateLagometer( int aheadOfServer, int dupeUsercmds );

	virtual void			GetMapLoadingGUI( char gui[ MAX_STRING_CHARS ] );
};

//============================================================================

// HUMANHEAD pdm
// HUMANHEAD mdl:  Commented out the old gameLocal because it was messing up tagging
//#ifdef HUMANHEAD
#include "Prey/prey_game.h"
extern hhGameLocal			gameLocal;
//#else	// HUMANHEAD
//extern idGameLocal			gameLocal;
//#endif	// HUMANHEAD

extern idAnimManager		animationLib;

//============================================================================

class idGameError : public idException {
public:
	idGameError( const char *text ) : idException( text ) {}
};

//============================================================================

template< class type >
ID_INLINE idEntityPtr<type>::idEntityPtr()
{
    spawnId = 0;
}

template< class type >
ID_INLINE void idEntityPtr<type>::Save( idSaveGame* savefile ) const
{
    savefile->WriteInt( spawnId );
}

template< class type >
ID_INLINE void idEntityPtr<type>::Restore( idRestoreGame* savefile )
{
    savefile->ReadInt( spawnId );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::SetSpawnId( int id ) {
	// the reason for this first check is unclear:
	// the function returning false may mean the spawnId is already set right, or the entity is missing
	if ( id == spawnId ) {
		return false;
	}
	if ( ( id >> GENTITYNUM_BITS ) == gameLocal.spawnIds[ id & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] ) {
		spawnId = id;
		return true;
	}
	return false;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsValid( void ) const {
	return ( gameLocal.spawnIds[ spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 ) ] == ( spawnId >> GENTITYNUM_BITS ) );
}

template< class type >
ID_INLINE type *idEntityPtr<type>::GetEntity( void ) const {
	int entityNum = spawnId & ( ( 1 << GENTITYNUM_BITS ) - 1 );
	if ( gameLocal.spawnIds[ entityNum ] == ( spawnId >> GENTITYNUM_BITS ) ) {
		return static_cast<type *>( gameLocal.entities[ entityNum ] );
	}
	return NULL;
}

template< class type >
ID_INLINE int idEntityPtr<type>::GetEntityNum( void ) const {
	//HUMANHEAD rww - take cent bits into account
	return ( spawnId & ( ( 1 << GENTITYNUM_BITS_PLUSCENT ) - 1 ) );
}

//============================================================================

//
// these defines work for all startsounds from all entity types
// make sure to change script/doom_defs.script if you add any channels, or change their order
//
typedef enum {
	SND_CHANNEL_ANY = SCHANNEL_ANY,
	SND_CHANNEL_VOICE = SCHANNEL_ONE,
	SND_CHANNEL_VOICE2,
	SND_CHANNEL_BODY,
	SND_CHANNEL_BODY2,
	SND_CHANNEL_BODY3,
	SND_CHANNEL_WEAPON,
	SND_CHANNEL_ITEM,
	SND_CHANNEL_HEART,
	SND_CHANNEL_PDA,
	SND_CHANNEL_LANDING,	// HUMANHEAD pdm: needed seperate channel for landing
	SND_CHANNEL_DEMONIC,
	SND_CHANNEL_RADIO,

//HUMANHEAD: aob
	SND_CHANNEL_WALLWALK,
	SND_CHANNEL_WALLWALK2,
	SND_CHANNEL_SPIRITWALK,
	SND_CHANNEL_MISC1,
	SND_CHANNEL_MISC2,
	SND_CHANNEL_MISC3,
	SND_CHANNEL_MISC4,
	SND_CHANNEL_MISC5,
	SND_CHANNEL_IDLE,
	SND_CHANNEL_DYING,
	SND_CHANNEL_THRUSTERS,
	SND_CHANNEL_DOCKED,
	SND_CHANNEL_TRACTORBEAM,
	SND_CHANNEL_RECHARGE,
//HUMANHEAD END

	// internal use only.  not exposed to script or framecommands.
	SND_CHANNEL_AMBIENT,
	SND_CHANNEL_DAMAGE
} gameSoundChannel_t;

extern const float	DEFAULT_GRAVITY;
extern const idVec3	DEFAULT_GRAVITY_VEC3;
extern const int	CINEMATIC_SKIP_DELAY;


//============================================================================

#include "physics/Force.h"
#include "physics/Force_Constant.h"
#include "physics/Force_Drag.h"
#include "physics/Force_Field.h"
#include "physics/Force_Spring.h"
#include "physics/Physics.h"
#include "physics/Physics_Static.h"
#include "physics/Physics_StaticMulti.h"
#include "physics/Physics_Base.h"
#include "physics/Physics_Actor.h"
#include "physics/Physics_Monster.h"
#include "physics/Physics_Player.h"
#include "physics/Physics_Parametric.h"
#include "physics/Physics_RigidBody.h"
#include "physics/Physics_AF.h"

//HUMANHEAD
#include "physics/Physics_PreyPlayer.h"			// HUMANHEAD
#include "Prey/physics_preyparametric.h"		// HUMANHEAD nla
#include "Prey/physics_preyai.h"				// HUMANHEAD jrm
#include "Prey/physics_simple.h"				// HUMANHEAD aob
#include "Prey/game_woundmanager.h"			// HUMANHEAD: aob - must be before Entity.h
//HUMANHEAD END

#include "SmokeParticles.h"

#include "Entity.h"

//HUMANHEAD
//#include "../prey/game_renderentity.h"			// should be right after idRenderEntity.h
#include "Prey/game_animatedentity.h"		// should be right after entity.h
//HUMANHEAD END

#include "GameEdit.h"
#include "AF.h"
#include "IK.h"
#include "AFEntity.h"
#include "Misc.h"

// HUMANHEAD
#include "ai/AAS_local.h"						// HUMANHEAD nla
// HUMANHEAD END

#include "Actor.h"
#include "Projectile.h"

//HUMANHEAD:
#include "Prey/game_fxinfo.h"				// HUMANHEAD
#include "Prey/particles_particles.h"
#include "Prey/game_spherepart.h"			//HUMANHEAD: aob
#include "Prey/physics_delta.h"				//HUMANHEAD: aob
#include "Prey/game_animDriven.h"			//HUMANHEAD: aob
#include "Prey/prey_projectile.h"			//HUMANHEAD: aob
#include "Prey/prey_projectileautocannon.h"	//HUMANHEAD: aob
#include "Prey/prey_projectilerifle.h"		//HUMANHEAD: aob
#include "Prey/prey_projectilesoulcannon.h"		//HUMANHEAD: aob
#include "Prey/prey_projectiletracking.h"		//HUMANHEAD: aob
#include "Prey/prey_projectilerocketlauncher.h"	//HUMANHEAD: aob
#include "Prey/prey_projectilehiderweapon.h"		//HUMANHEAD: aob
#include "Prey/prey_projectilespiritarrow.h"		//HUMANHEAD: aob
#include "Prey/prey_projectilecrawlergrenade.h"	//HUMANHEAD: aob
#include "Prey/prey_projectileshuttle.h"			//HUMANHEAD: aob
#include "Prey/prey_projectilemine.h"		//HUMANHEAD: aob
#include "Prey/prey_projectilebugtrigger.h"			//HUMANHEAD: jsh
#include "Prey/prey_projectilebug.h"			//HUMANHEAD: jsh
#include "Prey/prey_projectilecocoon.h"			//HUMANHEAD: jsh
#include "Prey/prey_projectilegasbagpod.h"		//HUMANHEAD: mdl
#include "Prey/prey_projectilebounce.h"	//HUMANHEAD: aob
#include "Prey/prey_projectiletrigger.h"			//HUMANHEAD: jsh
#include "Prey/prey_projectilefreezer.h"			//HUMANHEAD: bjk
#include "Prey/prey_projectilewrench.h"			//HUMANHEAD: bjk
#include "Item.h"								//HUMANHEAD: aob - must be above weapon.h
#include "Prey/game_dda.h"
#include "Prey/game_utils.h"					//HUMANHEAD: aob
#include "Prey/prey_soundleadincontroller.h" //HUMANHEAD: aob
//HUMANHEAD END

#include "Weapon.h"
#include "Light.h"
#include "WorldSpawn.h"
// HUMANHEAD AOB: Moved up above weapon
#include "PlayerView.h"
#include "PlayerIcon.h"

// HUMANHEAD
#include "Prey/game_light.h"					// HUMANHEAD: aob
#include "Prey/game_playerview.h"
#include "Prey/prey_firecontroller.h"		// HUMANHEAD: aob
#include "Prey/prey_weaponfirecontroller.h"	// HUMANHEAD: aob
#include "Prey/prey_vehiclefirecontroller.h"	// HUMANHEAD: aob
#include "Prey/prey_baseweapons.h"
#include "Prey/prey_items.h"					// HUMANHEAD: aob
#include "Prey/game_hand.h"					// HUMANHEAD nla - must be before guihand
#include "Prey/game_guihand.h"				// HUMANHEAD nla - must be before Player
#include "Prey/game_handcontrol.h"			// HUMANHEAD pdm - must be before Player
#include "Prey/game_weaponHandState.h"
// HUMANHEAD END

#include "Player.h"
#include "Prey/physics_vehicle.h"			// HUMANHEAD pdm - must be before game_vehicle.h
#include "Prey/force_converge.h"				// HUMANHEAD pdm - must be before game_vehicle.h
#include "Prey/game_vehicle.h"				// HUMANHEAD pdm - must be before game_player.h
#include "Prey/game_dock.h"					// HUMANHEAD
#include "Prey/game_shuttle.h"				// HUMANHEAD pdm - must be after game_vehicle.h
#include "Prey/game_player.h"				// HUMANHEAD nla
#include "Mover.h"
#include "Camera.h"
#include "Moveable.h"
#include "Target.h"
#include "Trigger.h"
#include "Sound.h"
#include "Prey/prey_sound.h"//HUMANHEAD: aob
#include "Prey/prey_spiritproxy.h"			// HUMANHEAD
#include "Fx.h"
#include "Prey/game_entityfx.h"				// HUMANHEAD
#include "SecurityCamera.h"
#include "BrittleFracture.h"

#include "ai/AI.h"
#include "anim/Anim_Testmodel.h"

#include "script/Script_Compiler.h"
#include "script/Script_Interpreter.h"
#include "script/Script_Thread.h"

//HUMANHEAD: aob - must be after Script_Thread.h
#include "Prey/prey_script_thread.h"
//HUMANHEAD END

//HUMANHEAD: aob - moved operator= code to this helper function
#ifdef HUMANHEAD
template< class type >
ID_INLINE void idEntityPtr<type>::Clear() {
	spawnId = 0;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsEqualTo( const idEntity *ent ) const {
	return GetEntity() == ent;
}

template< class type >
ID_INLINE bool idEntityPtr<type>::IsEqualTo( const idEntityPtr<type> &ent ) const {
	return IsEqualTo( ent.GetEntity() );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator==( const idEntity *ent ) const {
	return IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator==( const idEntityPtr<type> &ent ) const {
	return IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator!=( const idEntity *ent ) const {
	return !IsEqualTo( ent );
}

template< class type >
ID_INLINE bool idEntityPtr<type>::operator!=( const idEntityPtr<type> &ent ) const {
	return !IsEqualTo( ent );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::Assign( const idEntityPtr<type> &ent ) {
	return Assign( ent.GetEntity() );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::Assign( const idEntity *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		//HUMANHEAD rww - take cent bits into account
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS_PLUSCENT ) | ent->entityNumber;
	}
	return *this;
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( const idEntityPtr<type> &ent ) {
	return Assign( ent );
}

template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( const idEntity *ent ) {
	return Assign( ent );
}
#else
template< class type >
ID_INLINE idEntityPtr<type> &idEntityPtr<type>::operator=( type *ent ) {
	if ( ent == NULL ) {
		spawnId = 0;
	} else {
		spawnId = ( gameLocal.spawnIds[ent->entityNumber] << GENTITYNUM_BITS ) | ent->entityNumber;
	}
	return *this;
}
#endif
// HUMANHEAD END

#endif	/* !__GAME_LOCAL_H__ */
