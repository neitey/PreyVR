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

#ifndef __GAME_PROJECTILE_H__
#define __GAME_PROJECTILE_H__

#include "physics/Physics_RigidBody.h"
#include "physics/Force_Constant.h"
#include "Entity.h"

/*
===============================================================================

  idProjectile

===============================================================================
*/

//HUMANHEAD: aob
extern const idEventDef EV_Fizzle;
//HUMANHEAD END

extern const idEventDef EV_Explode;

class idProjectile : public idEntity {
public :
	CLASS_PROTOTYPE( idProjectile );

							idProjectile();
	virtual					~idProjectile();

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual //HUMANHEAD
	void					Create( idEntity *owner, const idVec3 &start, const idVec3 &dir );
	virtual void			Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f, const float motionThrowSpeed = 0.0f );
	virtual void			FreeLightDef( void );

	idEntity *				GetOwner( void ) const;
	void					CatchProjectile( idEntity* o, const char* reflectName );
    int						GetProjectileState();
	void					Event_CreateProjectile( idEntity* owner, const idVec3& start, const idVec3& dir );
	void					Event_LaunchProjectile( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity );
	void					Event_SetGravity( float gravity );

	virtual void			Think( void );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Explode( const trace_t &collision, idEntity *ignore );
	virtual //HUMANHEAD
	void					Fizzle( void );

	//HUMANHEAD: aob - implementation in hhProjectile
	virtual void			Create( idEntity *owner, const idVec3 &start, const idMat3 &axis ) {}
	virtual void			Launch( const idVec3 &start, const idMat3 &axis, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f ) {}
	//HUMANHEAD END

	static idVec3			GetVelocity( const idDict *projectile );
	static idVec3			GetGravity( const idDict *projectile );

	enum {
		EVENT_DAMAGE_EFFECT = idEntity::EVENT_MAXEVENTS,
		EVENT_PROJECTILE_EXPLOSION, //HUMANHEAD rww
		EVENT_MAXEVENTS
	};

	void					SetLaunchedFromGrabber( bool bl )
	{
		launchedFromGrabber = bl;
	}
	bool					GetLaunchedFromGrabber()
	{
		return launchedFromGrabber;
	}

	static void				DefaultDamageEffect( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity );
	static bool				ClientPredictionCollide( idEntity *soundEnt, const idDict &projectileDef, const trace_t &collision, const idVec3 &velocity, bool addDamageEffect );
	virtual void			ClientPredictionThink( void );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
	virtual bool			ClientReceiveEvent( int event, int time, const idBitMsg &msg );
	//HUMANHEAD rww
	virtual void			ClientHideProjectile(void);
	//HUMANHEAD END

protected:
	idEntityPtr<idEntity>	owner;

	struct projectileFlags_s {
		bool				detonate_on_world			: 1;
		bool				detonate_on_actor			: 1;
		bool				randomShaderSpin			: 1;
		bool				isTracer					: 1;
		bool				noSplashDamage				: 1;
		bool				isLarge						: 1;	// HUMANHEAD bjk
	} projectileFlags;

	bool					launchedFromGrabber;

	float					thrust;
	int						thrust_end;
	float					damagePower;

	renderLight_t			renderLight;
	qhandle_t				lightDefHandle;				// handle to renderer light def
	idVec3					lightOffset;
	int						lightStartTime;
	int						lightEndTime;
	idVec3					lightColor;

	//HUMANHEAD rww - don't need thruster objects on the projectile, takes up time for constructor
	//idForce_Constant		thruster;
	idPhysics_RigidBody		physicsObj;

	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;

	bool					mNoExplodeDisappear;
	bool					mTouchTriggers;
	int						originalTimeGroup;

	typedef enum {
		// must update these in script/doom_defs.script if changed
		SPAWNED = 0,
		CREATED = 1,
		LAUNCHED = 2,
		FIZZLED = 3,
		EXPLODED = 4,
		COLLIDED = 5	//HUMANHEAD bjk
	} projectileState_t;

	projectileState_t		state;

protected: //HUMANHEAD
	bool					netSyncPhysics;

	idVec3					launchOrigin;
	idMat3					launchAxis;

	void					AddDefaultDamageEffect( const trace_t &collision, const idVec3 &velocity );

	void					Event_Explode( void );
	void					Event_Fizzle( void );
	void					Event_RadiusDamage( idEntity *ignore );
	void					Event_Touch( idEntity *other, trace_t *trace );
	void					Event_GetProjectileState( void );
};

class idGuidedProjectile : public idProjectile {
public :
	CLASS_PROTOTYPE( idGuidedProjectile );

							idGuidedProjectile( void );
							~idGuidedProjectile( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn( void );
	virtual void			Think( void );
	virtual void			Launch( const idVec3& start, const idVec3& dir, const idVec3& pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f, const float motionThrowSpeed = 0.0f );
    void					SetEnemy( idEntity* ent );
    void					Event_SetEnemy( idEntity* ent );

protected:
	float					speed;
	idEntityPtr<idEntity>	enemy;
	virtual void			GetSeekPos( idVec3 &out );

private:
	idAngles				rndScale;
	idAngles				rndAng;
	idAngles				angles;
	int						rndUpdateTime;
	float					turn_max;
	float					clamp_dist;
	bool					burstMode;
	bool					unGuided;
	float					burstDist;
	float					burstVelocity;
};

// idSoulCubeMissile (HUMANHEAD pdm: removed)

// idBFGProjectile (HUMANHEAD pdm: removed)

class idHomingProjectile : public idProjectile {
public :
CLASS_PROTOTYPE( idHomingProjectile );

	idHomingProjectile();
	~idHomingProjectile();

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	void					Spawn();
	virtual void			Think();
	virtual void			Launch( const idVec3 &start, const idVec3 &dir, const idVec3 &pushVelocity, const float timeSinceFire = 0.0f, const float launchPower = 1.0f, const float dmgPower = 1.0f );
	void					SetEnemy( idEntity *ent );
	void					SetSeekPos( idVec3 pos );
	void					Event_SetEnemy(idEntity *ent);

protected:
	float					speed;
	idEntityPtr<idEntity>	enemy;
	idVec3					seekPos;

private:
	idAngles				rndScale;
	idAngles				rndAng;
	idAngles				angles;
	float					turn_max;
	float					clamp_dist;
	bool					burstMode;
	bool					unGuided;
	float					burstDist;
	float					burstVelocity;
};

/*
===============================================================================

  idDebris

===============================================================================
*/

class idDebris : public idEntity {
public :
	CLASS_PROTOTYPE( idDebris );

							idDebris();
							~idDebris();

	// save games
	void					Save( idSaveGame *savefile ) const;					// archives object for save game file
	void					Restore( idRestoreGame *savefile );					// unarchives object from save game file

	void					Spawn( void );

	void					Create( idEntity *owner, const idVec3 &start, const idMat3 &axis );
	void					Launch( void );
	void					Think( void );
	void					Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	void					Explode( void );
	void					Fizzle( void );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );

	//HUMANHEAD rww
	virtual void			ClientPredictionThink( void );
	void					WriteToSnapshot( idBitMsgDelta &msg ) const;
	void					ReadFromSnapshot( const idBitMsgDelta &msg );
	//HUMANHEAD END

//HUMANHEAD: aob
protected:
	s_channelType			DetermineNextChannel();
	void					AttemptToPlayBounceSound( const trace_t &collision, const idVec3 &velocity );
//HUMANHEAD END

private:
	idEntityPtr<idEntity>	owner;
	idPhysics_RigidBody		physicsObj;
	const idDeclParticle *	smokeFly;
	int						smokeFlyTime;
	const idSoundShader *	sndBounce;

	//HUMANHEAD: aob
	float					collisionSpeed_max;
	float					collisionSpeed_min;
	int						currentChannel;
	int						solidTest; // mdl
	//HUMANHEAD END

	void					Event_Explode( void );
	void					Event_Fizzle( void );
	void					Event_CheckClip( void ); // HUMANHEAD mdl
};

#endif /* !__GAME_PROJECTILE_H__ */
