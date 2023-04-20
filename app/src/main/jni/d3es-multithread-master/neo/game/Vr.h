#ifndef __VR_H__
#define __VR_H__

#include "../renderer/Image.h"
#include "../idlib/math/Quat.h"
#include "../idlib/math/Matrix.h"
#include "../idlib/Str.h"
#include "../idlib/math/Angles.h"
#include "../framework/CVarSystem.h"
#include "../../../Doom3Quest/VrClientInfo.h"

//Lubos BEGIN
extern idCVar vr_shakeAmplitude;
extern idCVar vr_crouchTriggerDist;
extern idCVar vr_hudType;
extern idCVar vr_vehicle3d;

#include <android/log.h>

#define ALOGE(...) __android_log_print( ANDROID_LOG_ERROR, "PreyVR", __VA_ARGS__ )
#define ALOGV(...) __android_log_print( ANDROID_LOG_VERBOSE, "PreyVR", __VA_ARGS__ )

void ApplyVRWeaponTransform(idMat3 &axis, idVec3& origin);
//Lubos END

#endif
