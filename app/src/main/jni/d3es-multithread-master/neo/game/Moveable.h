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

#ifndef __GAME_MOVEABLE_H__
#define __GAME_MOVEABLE_H__

#include "physics/Physics_RigidBody.h"
#include "script/Script_Thread.h"
#include "gamesys/Event.h"
#include "Entity.h"
#include "Player.h"
#include "Projectile.h"

/*
===============================================================================

  Entity using rigid body physics.

===============================================================================
*/

extern const idEventDef EV_BecomeNonSolid;
extern const idEventDef EV_IsAtRest;

class idMoveable : public idEntity {
public:
	CLASS_PROTOTYPE( idMoveable );

							idMoveable( void );
							~idMoveable( void );

	void					Spawn( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

	virtual void			Think( void );

	virtual void			Hide( void );
	virtual void			Show( void );

	bool					AllowStep( void ) const;
	void					EnableDamage( bool enable, float duration );
	virtual bool			Collide( const trace_t &collision, const idVec3 &velocity );
	virtual void			Killed( idEntity *inflictor, idEntity *attacker, int damage, const idVec3 &dir, int location );
	virtual void			WriteToSnapshot( idBitMsgDelta &msg ) const;
	virtual void			ReadFromSnapshot( const idBitMsgDelta &msg );
    void					SetAttacker( idEntity* ent );
    const idEntity* 		GetAttacker()
    {
        return attacker;
    }

protected:
	idPhysics_RigidBody		physicsObj;				// physics object
	idStr					brokenModel;			// model set when health drops down to or below zero
	idStr					damage;					// if > 0 apply damage to hit entities
	idStr					monsterDamage;
	idEntity				*attacker;
	idStr					fxCollide;				// fx system to start when collides with something
	int						nextCollideFxTime;		// next time it is ok to spawn collision fx
	float					minDamageVelocity;		// minimum velocity before moveable applies damage
	float					maxDamageVelocity;		// velocity at which the maximum damage is applied
	idCurve_Spline<idVec3> *initialSpline;			// initial spline path the moveable follows
	idVec3					initialSplineDir;		// initial relative direction along the spline path
// HUMANHEAD pdm: unused
//	bool					explode;				// entity explodes when health drops down to or below zero
	bool					unbindOnDeath;			// unbind from master when health drops down to or below zero
	bool					allowStep;				// allow monsters to step on the object
	bool					canDamage;				// only apply damage when this is set
	int						nextDamageTime;			// next time the movable can hurt the player
	int						nextSoundTime;			// next time the moveable can make a sound

	const idMaterial *		GetRenderModelMaterial( void ) const;
	void					BecomeNonSolid( void );
	void					InitInitialSpline( int startTime );
	bool					FollowInitialSplinePath( void );

	virtual	// HUMANHEAD: made virtual
	void					Event_Activate( idEntity *activator );
	void					Event_BecomeNonSolid( void );
	void					Event_SetOwnerFromSpawnArgs( void );
	void					Event_IsAtRest( void );
	void					Event_EnableDamage( float enable );
};


/*
===============================================================================

  A barrel using rigid body physics. The barrel has special handling of
  the view model orientation to make it look like it rolls instead of slides.

  HUMANHEAD pdm: removed, re-inherited from hhMoveable

===============================================================================
*/


/*
===============================================================================

  A barrel using rigid body physics and special handling of the view model
  orientation to make it look like it rolls instead of slides. The barrel
  can burn and explode when damaged.

  HUMANHEAD pdm: removed, unused

===============================================================================
*/


#endif /* !__GAME_MOVEABLE_H__ */
