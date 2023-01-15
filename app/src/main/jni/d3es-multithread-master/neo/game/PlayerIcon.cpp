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

#include "idlib/precompiled.h"
#include "renderer/ModelManager.h"

#include "Player.h"

#include "PlayerIcon.h"

static const char * iconKeys[ ICON_NONE ] = {
	"mtr_icon_lag",
	"mtr_icon_chat",
	"mtr_icon_sameteam", //HUMANHEAD rww
	"mtr_icon_redteam",
	"mtr_icon_blueteam"
};

/*
===============
idPlayerIcon::idPlayerIcon
===============
*/
idPlayerIcon::idPlayerIcon() {
	iconHandle	= -1;
	iconType	= ICON_NONE;
}

/*
===============
idPlayerIcon::~idPlayerIcon
===============
*/
idPlayerIcon::~idPlayerIcon() {
	FreeIcon();
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idActor *player, jointHandle_t joint ) { //HUMANHEAD rww - general actor support
	idVec3 origin;
	idMat3 axis;
	float zOff = 16.0f; //HUMANHEAD rww

	if (player->InVehicle()) { //HUMANHEAD rww
		zOff = 64.0f;
	}

	if ( joint == INVALID_JOINT ) {
		//HUMANHEAD rww - invalid joint now valid for prox ent
		//FreeIcon();
		//return;
		axis = player->GetAxis();
		origin = player->GetOrigin();
		zOff += 70.0f;
	}
	else {
		player->GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	}
	//origin.z += 16.0f;
	origin += player->GetAxis()[2]*zOff; //HUMANEAD rww

	Draw( player, origin );
}

/*
===============
idPlayerIcon::Draw
===============
*/
void idPlayerIcon::Draw( idActor *player, const idVec3 &origin ) { //HUMANHEAD rww - general actor support
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer || !localPlayer->GetRenderView() ) {
		FreeIcon();
		return;
	}

	idMat3 axis = localPlayer->GetRenderView()->viewaxis;

	//HUMANHEAD rww - work for general actors
	hhPlayer *hhPl = NULL;
	bool isLagged = false;
	bool isChatting = false;
	if (player->IsType(hhPlayer::Type)) {
		hhPl = static_cast<hhPlayer *>(player);
	}
	//we could possibly show these icons for the prox, but i can see it causing issues when the player is out of the
	//snapshot and the prox is not, and i don't want to sync those states onto the prox ent as well.
	/*
	else if (player->IsType(hhSpiritProxy::Type)) {
		hhSpiritProxy *prox = static_cast<hhSpiritProxy *>(player);
		hhPl = prox->GetPlayer();
	}
	*/

	if (hhPl) {
		isLagged = hhPl->isLagged;
		isChatting = hhPl->isChatting;
	}
	//HUMANHEAD END

	if ( isLagged ) { //HUMANHEAD rww
		// create the icon if necessary, or update if already created
		if ( !CreateIcon( player, ICON_LAG, origin, axis ) ) {
			UpdateIcon( player, origin, axis );
		}
	} else if ( isChatting ) { //HUMANHEAD rww
		if ( !CreateIcon( player, ICON_CHAT, origin, axis ) ) {
			UpdateIcon( player, origin, axis );
		}
	} else {
		FreeIcon();
	}
}

/*
===============
idPlayerIcon::FreeIcon
===============
*/
void idPlayerIcon::FreeIcon( void ) {
	if ( iconHandle != - 1 ) {
		gameRenderWorld->FreeEntityDef( iconHandle );
		iconHandle = -1;
	}
	iconType = ICON_NONE;
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idActor *player, playerIconType_t type, const idVec3 &origin, const idMat3 &axis ) { //HUMANHEAD rww - general actor support
	assert( type != ICON_NONE );
	//HUMANHEAD rww
	hhPlayer *hhPl = NULL;
	if (player->IsType(hhPlayer::Type)) {
		hhPl = static_cast<hhPlayer *>(player);
	}
	else if (player->IsType(hhSpiritProxy::Type)) {
		hhSpiritProxy *prox = static_cast<hhSpiritProxy *>(player);
		hhPl = prox->GetPlayer();
	}
	if (!hhPl) {
		return false;
	}
	//HUMANHEAD END
	const char *mtr = hhPl->spawnArgs.GetString( iconKeys[ type ], "_default" );
	return CreateIcon( player, type, mtr, origin, axis );
}

/*
===============
idPlayerIcon::CreateIcon
===============
*/
bool idPlayerIcon::CreateIcon( idActor *player, playerIconType_t type, const char *mtr, const idVec3 &origin, const idMat3 &axis ) { //HUMANHEAD rww - general actor support
	assert( type != ICON_NONE );

	if ( type == iconType ) {
		return false;
	}

	FreeIcon();

	memset( &renderEnt, 0, sizeof( renderEnt ) );
	renderEnt.origin	= origin;
	renderEnt.axis		= axis;
	renderEnt.shaderParms[ SHADERPARM_RED ]				= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_GREEN ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_BLUE ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_ALPHA ]			= 1.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= 16.0f;
	renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= 16.0f;
	renderEnt.hModel = renderModelManager->FindModel( "_sprite" );
	renderEnt.callback = NULL;
	renderEnt.numJoints = 0;
	renderEnt.joints = NULL;
	renderEnt.customSkin = 0;
	renderEnt.noShadow = true;
	renderEnt.noSelfShadow = true;
	renderEnt.customShader = declManager->FindMaterial( mtr );
	renderEnt.referenceShader = 0;
	renderEnt.bounds = renderEnt.hModel->Bounds( &renderEnt );

	iconHandle = gameRenderWorld->AddEntityDef( &renderEnt );
	iconType = type;

	return true;
}

/*
===============
idPlayerIcon::UpdateIcon
===============
*/
void idPlayerIcon::UpdateIcon( idActor *player, const idVec3 &origin, const idMat3 &axis ) { //HUMANHEAD rww - general actor support
	assert( iconHandle >= 0 );

	renderEnt.origin = origin;
	renderEnt.axis	= axis;
	gameRenderWorld->UpdateEntityDef( iconHandle, &renderEnt );
}

//HUMANHEAD rww
/*
===============
hhPlayerTeamIcon::Draw
for some reason i have to override this too or the compiler directs joint calls
to the idVec3 version and uses the int as a mask. which is dumb.
===============
*/
void hhPlayerTeamIcon::Draw( idActor *player, jointHandle_t joint ) {
	idVec3 origin;
	idMat3 axis;
	float zOff = 32.0f;

	if (player->InVehicle()) {
		zOff = 80.0f;
	}

	if ( joint == INVALID_JOINT ) {
		//HUMANHEAD rww - invalid joint now valid for prox ent
		//FreeIcon();
		//return;
		axis = player->GetAxis();
		origin = player->GetOrigin();
		zOff += 70.0f;
	}
	else {
		player->GetJointWorldTransform( joint, gameLocal.time, origin, axis );
	}
	origin += player->GetAxis()[2]*zOff;

	Draw( player, origin );
}

/*
===============
hhPlayerTeamIcon::Draw
===============
*/
void hhPlayerTeamIcon::Draw( idActor *player, const idVec3 &origin ) {
	idPlayer *localPlayer = gameLocal.GetLocalPlayer();
	if ( !localPlayer || !localPlayer->GetRenderView() || localPlayer->spectating ) { //also don't draw team icons for spectators
		FreeIcon();
		return;
	}

	hhPlayer *hhPl = NULL;
	int plTeam = 0;
	if (player->IsType(hhPlayer::Type)) {
		hhPl = static_cast<hhPlayer *>(player);
	}
	else if (player->IsType(hhSpiritProxy::Type)) {
		hhSpiritProxy *prox = static_cast<hhSpiritProxy *>(player);
		hhPl = prox->GetPlayer();
	}

	if (hhPl) {
		plTeam = hhPl->team;
	}

	if ( gameLocal.gameType == GAME_TDM && plTeam == localPlayer->team ) {
		idMat3 axis = localPlayer->GetRenderView()->viewaxis;

		// create the icon if necessary, or update if already created
		CreateIcon( player, ICON_SAMETEAM, origin, axis );
		if (player->InVehicle()) {
			renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= 48.0f; //32
			renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= 48.0f; //32
		}
		else {
			renderEnt.shaderParms[ SHADERPARM_SPRITE_WIDTH ]	= 32.0f; //16
			renderEnt.shaderParms[ SHADERPARM_SPRITE_HEIGHT ]	= 32.0f; //16
		}
		UpdateIcon( player, origin, axis );
	}
	else {
		FreeIcon();
	}
}
//HUMANHEAD END
