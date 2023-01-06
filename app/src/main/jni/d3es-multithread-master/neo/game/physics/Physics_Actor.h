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

#ifndef __PHYSICS_ACTOR_H__
#define __PHYSICS_ACTOR_H__

#include "Game_local.h"
#include "physics/Physics_Base.h"

//HUMANHEAD
#define	MAXTOUCH					32
const int c_iNumRotationTraces		= 4;
//HUMANHEAD END

/*
===================================================================================

	Actor physics base class

	An actor typically uses one collision model which is aligned with the gravity
	direction. The collision model is usually a simple box with the origin at the
	bottom center.

===================================================================================
*/

class idPhysics_Actor : public idPhysics_Base {

public:
	CLASS_PROTOTYPE( idPhysics_Actor );

							idPhysics_Actor( void );
							~idPhysics_Actor( void );

	void					Save( idSaveGame *savefile ) const;
	void					Restore( idRestoreGame *savefile );

							// get delta yaw of master
	float					GetMasterDeltaYaw( void ) const;
							// returns the ground entity
	idEntity *				GetGroundEntity( void ) const;
							// align the clip model with the gravity direction
	void					SetClipModelAxis( void );

	//HUMANHEAD nla
	int						GetNumTouchEnts( void ) const;
	int						GetTouchEnt( int i ) const;
	void					ResetNumTouchEnt( int i );
	// HUMANHEAD END

	//HUMANHEAD: aob - added
	const trace_t&			GetGroundTrace() const { return groundTrace; }
	virtual const idMaterial*	GetGroundMaterial() const { if(!HasGroundContacts()) return NULL; return groundTrace.c.material; } // JRM - return NULL if no gc's
	surfTypes_t				GetGroundMatterType() const { return gameLocal.GetMatterType(groundTrace); }
	const int				GetGroundSurfaceFlags() const { const idMaterial* mat = GetGroundMaterial(); if( mat ) { return mat->GetSurfaceFlags(); } else { return 0; } }
	bool					HadGroundContacts() const;
	virtual void			HadGroundContacts( const bool hadGroundContacts );
	//HUMANHEAD END

public:	// common physics interface
	void					SetClipModel( idClipModel *model, float density, int id = 0, bool freeOld = true );
	idClipModel *			GetClipModel( int id = 0 ) const;
	int						GetNumClipModels( void ) const;

	void					SetMass( float mass, int id = -1 );
	float					GetMass( int id = -1 ) const;

	void					SetContents( int contents, int id = -1 );
	int						GetContents( int id = -1 ) const;

	const idBounds &		GetBounds( int id = -1 ) const;
	const idBounds &		GetAbsBounds( int id = -1 ) const;

	bool					IsPushable( void ) const;

	const idVec3 &			GetOrigin( int id = 0 ) const;
	const idMat3 &			GetAxis( int id = 0 ) const;

	virtual // HUMANHEAD jsh made virtual
	void					SetGravity( const idVec3 &newGravity );
	const idMat3 &			GetGravityAxis( void ) const;

	void					ClipTranslation( trace_t &results, const idVec3 &translation, const idClipModel *model ) const;
	void					ClipRotation( trace_t &results, const idRotation &rotation, const idClipModel *model ) const;
	int						ClipContents( const idClipModel *model ) const;

	void					DisableClip( void );
	void					EnableClip( void );

	void					UnlinkClip( void );
	void					LinkClip( void );

	bool					EvaluateContacts( void );

protected:
	idClipModel *			clipModel;			// clip model used for collision detection
	idMat3					clipModelAxis;		// axis of clip model aligned with gravity direction

	// derived properties
	float					mass;
	float					invMass;

	// master
	idEntity *				masterEntity;
	float					masterYaw;
	float					masterDeltaYaw;

	//HUMANHEAD: aob
	trace_t					groundTrace;
	//HUMANHEAD END

	//HUMANHEAD nla
	// entities touched during last evaluate
	int						numTouch;
	int						touchEnts[MAXTOUCH];
	void					AddTouchEnt( int entityNum );
	void					AddTouchEntList( idList<int> &list );

	bool					hadGroundContacts;

	idVec3					rotationTraceDirectionTable[ c_iNumRotationTraces ];
	// HUMANHEAD END

	// results of last evaluate
	idEntityPtr<idEntity>	groundEntityPtr;

//HUMANHEAD: aob
protected:
	virtual bool			IterativeRotateMove( const idVec3& upVector, const idVec3& idealUpVector, const idVec3& rotationOrigin, const idVec3& rotationCheckOrigin, int numIterations );
	idVec3					DetermineRotationVector( const idVec3& upVector, const idVec3& idealUpVector, const idVec3& rotationOrigin );
	bool					DirectionIsClear( const idVec3& checkOrigin, const idVec3& direction );
	void					BuildRotationTraceDirectionTable( const idMat3& Axis );
//HUMANHEAD END
};

#endif /* !__PHYSICS_ACTOR_H__ */
