/************************************************************************************

Filename	:	VrInputDefault.c
Content		:	Handles default controller input
Created		:	August 2019
Authors		:	Simon Brown, Lubos Vonasek

*************************************************************************************/

#include <android/keycodes.h>
#include <sys/time.h>

#include "VrInput.h"

#include "doomkeys.h"
#include "VrCommon.h"

float	vr_weapon_pitchadjust = -30.0f;

extern bool forceVirtualScreen;

uint32_t leftTrackedRemoteState_old;
uint32_t leftTrackedRemoteState_new;
uint32_t rightTrackedRemoteState_old;
uint32_t rightTrackedRemoteState_new;

float remote_movementSideways;
float remote_movementForward;


void Android_SetImpulse(int impulse);
void Android_SetCommand(const char * cmd);
void Android_ButtonChange(int key, int state);
int Android_GetCVarInteger(const char* cvar);

extern bool inMenu;
extern bool inGameGuiActive;
extern bool objectiveSystemActive;
extern bool inCinematic;

//All this to allow stick and button switching!
XrVector2f pPrimaryJoystick;
XrVector2f pSecondaryJoystick;
uint32_t weaponButtonsNew;
uint32_t weaponButtonsOld;
uint32_t offhandButtonsNew;
uint32_t offhandButtonsOld;

void Doom3Quest_HapticEvent(const char* event, int position, int flags, int intensity, float angle, float yHeight );

void HandleInput_Default(int controlscheme, int switchsticks)
{
	int primaryController = 1 - controlscheme;
	int primaryThumbStick = controlscheme == 0 ? 1 - switchsticks : switchsticks;
	int domButton1 = controlscheme == 0 ? ovrButton_A : ovrButton_X;
	int domButton2 = controlscheme == 0 ? ovrButton_B : ovrButton_Y;
	int offButton1 = controlscheme == 1 ? ovrButton_A : ovrButton_X;
	int offButton2 = controlscheme == 1 ? ovrButton_B : ovrButton_Y;

    //Need this for the touch screen
    XrPosef pWeapon = IN_VRGetPose(primaryController);
    XrPosef pOff = IN_VRGetPose(1 - primaryController);

	weaponButtonsOld = weaponButtonsNew;
	offhandButtonsOld = offhandButtonsNew;
    weaponButtonsNew = IN_VRGetButtonState(primaryController);
    offhandButtonsNew = IN_VRGetButtonState(1 - primaryController);
	pPrimaryJoystick = IN_VRGetJoystickState(primaryThumbStick);
	pSecondaryJoystick = IN_VRGetJoystickState(1 - primaryThumbStick);
	rightTrackedRemoteState_new = weaponButtonsNew;
	leftTrackedRemoteState_new = offhandButtonsNew;

	//Store original values
	const XrQuaternionf quatRHand = pWeapon.orientation;
	const XrVector3f positionRHand = pWeapon.position;
	const XrQuaternionf quatLHand = pOff.orientation;
	const XrVector3f positionLHand = pOff.position;

    {
        //Right Hand
        //GB - FP Already does this so we end up with backward hands
        if(controlscheme == 1) {//Left Handed
            VectorSet(pVRClientInfo->lhandposition, positionRHand.x, positionRHand.y, positionRHand.z);
            Vector4Set(pVRClientInfo->lhand_orientation_quat, quatRHand.x, quatRHand.y, quatRHand.z, quatRHand.w);
            VectorSet(pVRClientInfo->rhandposition, positionLHand.x, positionLHand.y, positionLHand.z);
            Vector4Set(pVRClientInfo->rhand_orientation_quat, quatLHand.x, quatLHand.y, quatLHand.z, quatLHand.w);
        } else {
            VectorSet(pVRClientInfo->rhandposition, positionRHand.x, positionRHand.y, positionRHand.z);
            Vector4Set(pVRClientInfo->rhand_orientation_quat, quatRHand.x, quatRHand.y, quatRHand.z, quatRHand.w);
            VectorSet(pVRClientInfo->lhandposition, positionLHand.x, positionLHand.y, positionLHand.z);
            Vector4Set(pVRClientInfo->lhand_orientation_quat, quatLHand.x, quatLHand.y, quatLHand.z, quatLHand.w);
        }


        //Set gun angles - We need to calculate all those we might need (including adjustments) for the client to then take its pick
        vec3_t rotation = {0};
        rotation[PITCH] = 30;
        rotation[PITCH] = vr_weapon_pitchadjust;
        QuatToYawPitchRoll(quatRHand, rotation, pVRClientInfo->weaponangles_temp);
        VectorSubtract(pVRClientInfo->weaponangles_last_temp, pVRClientInfo->weaponangles_temp, pVRClientInfo->weaponangles_delta_temp);
        VectorCopy(pVRClientInfo->weaponangles_temp, pVRClientInfo->weaponangles_last_temp);
    }

    //Menu button - can be used in all modes
    handleTrackedControllerButton_AsKey(leftTrackedRemoteState_new, leftTrackedRemoteState_old, ovrButton_Enter, K_ESCAPE);
    handleTrackedControllerButton_AsKey(rightTrackedRemoteState_new, rightTrackedRemoteState_old, ovrButton_Enter, K_ESCAPE); // in case user has switched menu/home buttons

    controlMouse(inMenu);

    if ( inMenu || inCinematic ) // Specific cases where we need to interact using mouse etc
    {
	    //Lubos BEGIN
	    handleTrackedControllerButton_AsButton(leftTrackedRemoteState_new, leftTrackedRemoteState_old, true, ovrButton_Trigger, 1);
	    handleTrackedControllerButton_AsButton(rightTrackedRemoteState_new, rightTrackedRemoteState_old, true, ovrButton_Trigger, 1);
	    //Lubos END
    }

    if ( ( !inCinematic && !inMenu ) || ( pVRClientInfo->vehicleMode && !inMenu ) )
    {
        float controllerYawHeading = 0.0f;
        //GBFP - off-hand stuff
        {
            pVRClientInfo->offhandoffset_temp[0] = pOff.position.x - pVRClientInfo->hmdposition[0];
            pVRClientInfo->offhandoffset_temp[1] = pOff.position.y - pVRClientInfo->hmdposition[1];
            pVRClientInfo->offhandoffset_temp[2] = pOff.position.z - pVRClientInfo->hmdposition[2];

            vec3_t rotation = {0};
            rotation[PITCH] = -45;
            QuatToYawPitchRoll(pOff.orientation, rotation, pVRClientInfo->offhandangles_temp);
        }

        //Right-hand specific stuff
        {
            //Fire Primary
            if ((weaponButtonsNew & ovrButton_Trigger) !=
                (weaponButtonsOld & ovrButton_Trigger)) {

                ALOGV("**WEAPON EVENT**  Trigger Pushed %sattack", (weaponButtonsNew & ovrButton_Trigger) ? "+" : "-");

                handleTrackedControllerButton_AsButton(weaponButtonsNew, weaponButtonsOld, false, ovrButton_Trigger, UB_ATTACK);
            }

            //Lubos BEGIN
            if ((weaponButtonsNew & ovrButton_GripTrigger) !=
                (weaponButtonsOld & ovrButton_GripTrigger)) {

                ALOGV("**WEAPON EVENT**  Grip Pushed %sattack", (weaponButtonsNew & ovrButton_GripTrigger) ? "+" : "-");

                handleTrackedControllerButton_AsButton(weaponButtonsNew, weaponButtonsOld, false, ovrButton_GripTrigger, UB_ATTACK_ALT);
            }
            //Lubos END

            //Duck
            if ((weaponButtonsNew & domButton1) != (weaponButtonsOld & domButton1)) {
	            handleTrackedControllerButton_AsButton(weaponButtonsNew, weaponButtonsOld, false, domButton1, UB_DOWN);
            }

            //Jump
            if ((weaponButtonsNew & domButton2) != (weaponButtonsOld & domButton2)) {
                handleTrackedControllerButton_AsButton(weaponButtonsNew, weaponButtonsOld, false, domButton2, UB_UP);
            }

			//Weapon Chooser
			static bool itemSwitched = false;
			if (between(-0.2f, pPrimaryJoystick.x, 0.2f) &&
				(between(0.8f, pPrimaryJoystick.y, 1.0f) ||
				 between(-1.0f, pPrimaryJoystick.y, -0.8f)))
			{
				pVRClientInfo->weaponZooming = between(0.8f, pPrimaryJoystick.y, 1.0f) ? 1 : -1; //Lubos
				if (!itemSwitched) {
					if (between(0.8f, pPrimaryJoystick.y, 1.0f))
					{
					    //Previous Weapon
                        Android_SetImpulse(UB_IMPULSE15);
					}
					else
					{
					    //Next Weapon
                        Android_SetImpulse(UB_IMPULSE14);
					}
					itemSwitched = true;
				}
			} else if (fabsf(pPrimaryJoystick.y) < 0.66f) {//Lubos
				pVRClientInfo->weaponZooming = 0; //Lubos
				itemSwitched = false;
			}
        }

        {
            //Apply a filter and quadratic scaler so small movements are easier to make
            float dist = length(pSecondaryJoystick.x, pSecondaryJoystick.y);
            float nlf = nonLinearFilter(dist);
            float x = (nlf * pSecondaryJoystick.x);
            float y = (nlf * pSecondaryJoystick.y);

            pVRClientInfo->player_moving = (fabs(x) + fabs(y)) > 0.05f;

            //Adjust to be off-hand controller oriented
            vec2_t v;
            rotateAboutOrigin(x, y, controllerYawHeading, v);

            //Move a lot slower if scope is engaged
            float vr_movement_multiplier = 127;
            remote_movementSideways = v[0] *  vr_movement_multiplier;
            remote_movementForward = v[1] *  vr_movement_multiplier;


	        //Lubos BEGIN
            {
                if (((offhandButtonsNew & ovrButton_Joystick) !=
                     (offhandButtonsOld & ovrButton_Joystick)) &&
                    (offhandButtonsNew & ovrButton_Joystick)) {
                    Android_SetImpulse(UB_IMPULSE54); //spiritwalk
                }

                if (((offhandButtonsNew & offButton1) !=
                     (offhandButtonsOld & offButton1)) &&
                    (offhandButtonsNew & offButton1)) {
                    Android_SetImpulse(UB_IMPULSE16); //lighter
                }
                if (((offhandButtonsNew & offButton2) !=
                     (offhandButtonsOld & offButton2)) &&
                    (offhandButtonsNew & offButton2)) {
                    if ((offhandButtonsNew & ovrButton_Trigger) &&
                    (offhandButtonsNew & ovrButton_GripTrigger)) {
						if (weaponButtonsNew & ovrButton_GripTrigger)
							Android_SetCommand("noclip"); //noclip cheat
						else
							Android_SetCommand("god"); //god cheat
                    } else {
                        Android_SetImpulse(UB_IMPULSE25); //throw granade
                    }
                }
            }
			pVRClientInfo->weaponModifier = offhandButtonsNew & ovrButton_GripTrigger;
	        //Lubos END

            //We need to record if we have started firing primary so that releasing trigger will stop definitely firing, if user has pushed grip
            //in meantime, then it wouldn't stop the gun firing and it would get stuck
            handleTrackedControllerButton_AsButton(offhandButtonsNew, offhandButtonsOld, false, ovrButton_Trigger, UB_SPEED);

            int vr_turn_mode = Android_GetCVarInteger("vr_turnmode");
            float vr_turn_angle = Android_GetCVarInteger("vr_turnangle");

            //This fixes a problem with older thumbsticks misreporting the X value
            static float joyx[4] = {0};
            for (int j = 3; j > 0; --j)
                joyx[j] = joyx[j-1];
            joyx[0] = pPrimaryJoystick.x;
            float joystickX = (joyx[0] + joyx[1] + joyx[2] + joyx[3]) / 4.0f;


            //No snap turn when using mounted gun
            //Lubos BEGIN
            static int increaseSnap = true;
            {
                if (joystickX > 0.7f) {
                    if (increaseSnap) {
                        float turnAngle = vr_turn_mode ? (vr_turn_angle / 9.0f) : vr_turn_angle;
                        pVRClientInfo->snapTurn -= turnAngle;

                        if (vr_turn_mode == 0) {
                            increaseSnap = false;
                        }

                        if (pVRClientInfo->snapTurn < -180.0f) {
                            pVRClientInfo->snapTurn += 360.f;
                        }
                    }
                } else if (joystickX < 0.2f) {
                    increaseSnap = true;
                }

                static int decreaseSnap = true;
                if (joystickX < -0.7f) {
                    if (decreaseSnap) {

                        float turnAngle = vr_turn_mode ? (vr_turn_angle / 9.0f) : vr_turn_angle;
                        pVRClientInfo->snapTurn += turnAngle;

                        //If snap turn configured for less than 10 degrees
                        if (vr_turn_mode == 0) {
                            decreaseSnap = false;
                        }

                        if (pVRClientInfo->snapTurn > 180.0f) {
                            pVRClientInfo->snapTurn -= 360.f;
                        }

                    }
                } else if (joystickX > -0.2f) {
                    decreaseSnap = true;
                }
            }
            //Lubos END
        }
    }

    //Save state
    rightTrackedRemoteState_old = rightTrackedRemoteState_new;
    leftTrackedRemoteState_old = leftTrackedRemoteState_new;
}
