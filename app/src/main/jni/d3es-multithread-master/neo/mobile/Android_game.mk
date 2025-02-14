LOCAL_PATH := $(call my-dir)/../


include $(CLEAR_VARS)

LOCAL_MODULE := game

LOCAL_C_INCLUDES :=  \
$(SDL_INCLUDE_PATHS) \
$(D3QUEST_TOP_PATH)/neo/mobile \
$(D3QUEST_TOP_PATH)/neo/game \
$(SDL_INCLUDE_PATHS)


LOCAL_CPPFLAGS := -DGAME_DLL -fPIC -D_K_CLANG
LOCAL_CPPFLAGS += -std=c++11 -D__DOOM_DLL__ -frtti -fexceptions  -Wno-error=format-security
LOCAL_CPPFLAGS += -D_HUMANHEAD -DHUMANHEAD -D_PREY


LOCAL_CPPFLAGS += -Wno-sign-compare \
                  -Wno-switch \
                  -Wno-format-security \


# Not avaliable in Android untill N
LOCAL_CFLAGS := -DIOAPI_NO_64

LOCAL_CFLAGS +=  -fno-unsafe-math-optimizations -fno-strict-aliasing -fno-math-errno -fno-trapping-math -fsigned-char


src_idlib = \
	idlib/bv/Bounds.cpp \
	idlib/bv/Frustum.cpp \
	idlib/bv/Sphere.cpp \
	idlib/bv/Box.cpp \
	idlib/geometry/DrawVert.cpp \
	idlib/geometry/Winding2D.cpp \
	idlib/geometry/Surface_SweptSpline.cpp \
	idlib/geometry/Winding.cpp \
	idlib/geometry/Surface.cpp \
	idlib/geometry/Surface_Patch.cpp \
	idlib/geometry/TraceModel.cpp \
	idlib/geometry/JointTransform.cpp \
	idlib/hashing/CRC32.cpp \
	idlib/hashing/MD4.cpp \
	idlib/hashing/MD5.cpp \
	idlib/math/Angles.cpp \
	idlib/math/Lcp.cpp \
	idlib/math/Math.cpp \
	idlib/math/Matrix.cpp \
	idlib/math/Ode.cpp \
	idlib/math/Plane.cpp \
	idlib/math/Pluecker.cpp \
	idlib/math/Polynomial.cpp \
	idlib/math/Quat.cpp \
	idlib/math/Rotation.cpp \
	idlib/math/Simd.cpp \
	idlib/math/Simd_Generic.cpp \
	idlib/math/Simd_AltiVec.cpp \
	idlib/math/Simd_MMX.cpp \
	idlib/math/Simd_3DNow.cpp \
	idlib/math/Simd_SSE.cpp \
	idlib/math/Simd_SSE2.cpp \
	idlib/math/Simd_SSE3.cpp \
	idlib/math/Vector.cpp \
	idlib/BitMsg.cpp \
	idlib/LangDict.cpp \
	idlib/Lexer.cpp \
	idlib/Lib.cpp \
	idlib/containers/HashIndex.cpp \
	idlib/Dict.cpp \
	idlib/Str.cpp \
	idlib/Parser.cpp \
	idlib/MapFile.cpp \
	idlib/CmdArgs.cpp \
	idlib/Token.cpp \
	idlib/Base64.cpp \
	idlib/Timer.cpp \
	idlib/Heap.cpp \
	idlib/bv/Frustum_gcc.cpp \
    idlib/precompiled.cpp \
    idlib/math/prey_math.cpp \


src_game = \
    game/Prey/ai_centurion.cpp \
    game/Prey/ai_crawler.cpp \
    game/Prey/ai_creaturex.cpp \
    game/Prey/ai_droid.cpp \
    game/Prey/ai_gasbag_simple.cpp \
    game/Prey/ai_harvester_simple.cpp \
    game/Prey/ai_hunter_simple.cpp \
    game/Prey/ai_inspector.cpp \
    game/Prey/ai_jetpack_harvester_simple.cpp \
    game/Prey/ai_keeper_simple.cpp \
    game/Prey/ai_mutate.cpp \
    game/Prey/ai_mutilatedhuman.cpp \
    game/Prey/ai_passageway.cpp \
    game/Prey/ai_possessedTommy.cpp \
    game/Prey/ai_reaction.cpp \
    game/Prey/ai_spawncase.cpp \
    game/Prey/ai_speech.cpp \
    game/Prey/ai_sphereboss.cpp \
    game/Prey/anim_baseanim.cpp \
    game/Prey/force_converge.cpp \
    game/Prey/game_afs.cpp \
    game/Prey/game_alarm.cpp \
    game/Prey/game_anim.cpp \
    game/Prey/game_animBlend.cpp \
    game/Prey/game_animDriven.cpp \
    game/Prey/game_animatedentity.cpp \
    game/Prey/game_animatedgui.cpp \
    game/Prey/game_animator.cpp \
    game/Prey/game_arcadegame.cpp \
    game/Prey/game_barrel.cpp \
    game/Prey/game_bindController.cpp \
    game/Prey/game_blackjack.cpp \
    game/Prey/game_cards.cpp \
    game/Prey/game_cilia.cpp \
    game/Prey/game_console.cpp \
    game/Prey/game_damagetester.cpp \
    game/Prey/game_dda.cpp \
    game/Prey/game_deathwraith.cpp \
    game/Prey/game_debrisspawner.cpp \
    game/Prey/game_dock.cpp \
    game/Prey/game_dockedgun.cpp \
    game/Prey/game_door.cpp \
    game/Prey/game_eggspawner.cpp \
    game/Prey/game_energynode.cpp \
    game/Prey/game_entityfx.cpp \
    game/Prey/game_entityspawner.cpp \
    game/Prey/game_events.cpp \
    game/Prey/game_fixedpod.cpp \
    game/Prey/game_forcefield.cpp \
    game/Prey/game_fxinfo.cpp \
    game/Prey/game_gibbable.cpp \
    game/Prey/game_gravityswitch.cpp \
    game/Prey/game_guihand.cpp \
    game/Prey/game_gun.cpp \
    game/Prey/game_hand.cpp \
    game/Prey/game_handcontrol.cpp \
    game/Prey/game_healthbasin.cpp \
    game/Prey/game_healthspore.cpp \
    game/Prey/game_inventory.cpp \
    game/Prey/game_itemautomatic.cpp \
    game/Prey/game_itemcabinet.cpp \
    game/Prey/game_jukebox.cpp \
    game/Prey/game_jumpzone.cpp \
    game/Prey/game_light.cpp \
    game/Prey/game_lightfixture.cpp \
    game/Prey/game_mine.cpp \
    game/Prey/game_misc.cpp \
    game/Prey/game_modeldoor.cpp \
    game/Prey/game_modeltoggle.cpp \
    game/Prey/game_monster_ai.cpp \
    game/Prey/game_monster_ai_events.cpp \
    game/Prey/game_mountedgun.cpp \
    game/Prey/game_moveable.cpp \
    game/Prey/game_mover.cpp \
    game/Prey/game_note.cpp \
    game/Prey/game_organtrigger.cpp \
    game/Prey/game_player.cpp \
    game/Prey/game_playerview.cpp \
    game/Prey/game_pod.cpp \
    game/Prey/game_podspawner.cpp \
    game/Prey/game_poker.cpp \
    game/Prey/game_portal.cpp \
    game/Prey/game_portalframe.cpp \
    game/Prey/game_proxdoor.cpp \
    game/Prey/game_rail.cpp \
    game/Prey/game_railshuttle.cpp \
    game/Prey/game_renderentity.cpp \
    game/Prey/game_safeDeathVolume.cpp \
    game/Prey/game_securityeye.cpp \
    game/Prey/game_shuttle.cpp \
    game/Prey/game_shuttledock.cpp \
    game/Prey/game_shuttletransport.cpp \
    game/Prey/game_skybox.cpp \
    game/Prey/game_slots.cpp \
    game/Prey/game_sphere.cpp \
    game/Prey/game_spherepart.cpp \
    game/Prey/game_spring.cpp \
    game/Prey/game_sunCorona.cpp \
    game/Prey/game_talon.cpp \
    game/Prey/game_targetproxy.cpp \
    game/Prey/game_targets.cpp \
    game/Prey/game_trackmover.cpp \
    game/Prey/game_trigger.cpp \
    game/Prey/game_tripwire.cpp \
    game/Prey/game_utils.cpp \
    game/Prey/game_vehicle.cpp \
    game/Prey/game_vomiter.cpp \
    game/Prey/game_weaponHandState.cpp \
    game/Prey/game_woundmanager.cpp \
    game/Prey/game_wraith.cpp \
    game/Prey/game_zone.cpp \
    game/Prey/particles_particles.cpp \
    game/Prey/physics_delta.cpp \
    game/Prey/physics_preyai.cpp \
    game/Prey/physics_preyparametric.cpp \
    game/Prey/physics_simple.cpp \
    game/Prey/physics_vehicle.cpp \
    game/Prey/prey_animator.cpp \
    game/Prey/prey_baseweapons.cpp \
    game/Prey/prey_beam.cpp \
    game/Prey/prey_bonecontroller.cpp \
    game/Prey/prey_camerainterpolator.cpp \
    game/Prey/prey_firecontroller.cpp \
    game/Prey/prey_game.cpp \
    game/Prey/prey_items.cpp \
    game/Prey/prey_liquid.cpp \
    game/Prey/prey_local.cpp \
    game/Prey/prey_projectile.cpp \
    game/Prey/prey_projectileautocannon.cpp \
    game/Prey/prey_projectilebounce.cpp \
    game/Prey/prey_projectilebug.cpp \
    game/Prey/prey_projectilebugtrigger.cpp \
    game/Prey/prey_projectilecocoon.cpp \
    game/Prey/prey_projectilecrawlergrenade.cpp \
    game/Prey/prey_projectilefreezer.cpp \
    game/Prey/prey_projectilegasbagpod.cpp \
    game/Prey/prey_projectilehiderweapon.cpp \
    game/Prey/prey_projectilemine.cpp \
    game/Prey/prey_projectilerifle.cpp \
    game/Prey/prey_projectilerocketlauncher.cpp \
    game/Prey/prey_projectileshuttle.cpp \
    game/Prey/prey_projectilesoulcannon.cpp \
    game/Prey/prey_projectilespiritarrow.cpp \
    game/Prey/prey_projectiletracking.cpp \
    game/Prey/prey_projectiletrigger.cpp \
    game/Prey/prey_projectilewrench.cpp \
    game/Prey/prey_script_thread.cpp \
    game/Prey/prey_sound.cpp \
    game/Prey/prey_soundleadincontroller.cpp \
    game/Prey/prey_spiritbridge.cpp \
    game/Prey/prey_spiritproxy.cpp \
    game/Prey/prey_spiritsecret.cpp \
    game/Prey/prey_vehiclefirecontroller.cpp \
    game/Prey/prey_weapon.cpp \
    game/Prey/prey_weaponautocannon.cpp \
    game/Prey/prey_weaponcrawlergrenade.cpp \
    game/Prey/prey_weaponfirecontroller.cpp \
    game/Prey/prey_weaponhider.cpp \
    game/Prey/prey_weaponrifle.cpp \
    game/Prey/prey_weaponrocketlauncher.cpp \
    game/Prey/prey_weaponsoulstripper.cpp \
    game/Prey/prey_weaponspiritbow.cpp \
    game/Prey/sys_debugger.cpp \
    game/Prey/sys_preycmds.cpp \
	game/ai/AAS.cpp \
	game/ai/AAS_debug.cpp \
	game/ai/AAS_pathing.cpp \
	game/ai/AAS_routing.cpp \
	game/ai/AI.cpp \
	game/ai/AI_events.cpp \
	game/ai/AI_pathing.cpp \
	game/anim/Anim.cpp \
	game/anim/Anim_Blend.cpp \
	game/anim/Anim_Import.cpp \
	game/anim/Anim_Testmodel.cpp \
	game/gamesys/DebugGraph.cpp \
	game/gamesys/Class.cpp \
	game/gamesys/Event.cpp \
	game/gamesys/SaveGame.cpp \
	game/gamesys/SysCmds.cpp \
	game/gamesys/SysCvar.cpp \
	game/gamesys/TypeInfo.cpp \
	game/physics/Clip.cpp \
	game/physics/Force.cpp \
	game/physics/Force_Constant.cpp \
	game/physics/Force_Drag.cpp \
	game/physics/Force_Field.cpp \
	game/physics/Force_Spring.cpp \
	game/physics/Physics.cpp \
	game/physics/Physics_AF.cpp \
	game/physics/Physics_Actor.cpp \
	game/physics/Physics_Base.cpp \
	game/physics/Physics_Monster.cpp \
	game/physics/Physics_Parametric.cpp \
	game/physics/Physics_Player.cpp \
	game/physics/Physics_PreyPlayer.cpp \
	game/physics/Physics_RigidBody.cpp \
	game/physics/Physics_Static.cpp \
	game/physics/Physics_StaticMulti.cpp \
	game/physics/Push.cpp \
	game/script/Script_Compiler.cpp \
	game/script/Script_Interpreter.cpp \
	game/script/Script_Program.cpp \
	game/script/Script_Thread.cpp \
	game/AF.cpp \
	game/AFEntity.cpp \
	game/Actor.cpp \
	game/BrittleFracture.cpp \
	game/Camera.cpp \
	game/Entity.cpp \
	game/EntityAdditions.cpp \
	game/Fx.cpp \
	game/GameEdit.cpp \
	game/Game_local.cpp \
	game/Game_network.cpp \
	game/Item.cpp \
	game/IK.cpp \
	game/Light.cpp \
	game/Misc.cpp \
	game/Mover.cpp \
	game/Moveable.cpp \
	game/MultiplayerGame.cpp \
	game/Player.cpp \
	game/PlayerIcon.cpp \
	game/PlayerView.cpp \
	game/Projectile.cpp \
	game/Pvs.cpp \
	game/SecurityCamera.cpp \
	game/SmokeParticles.cpp \
	game/Sound.cpp \
	game/Target.cpp \
	game/Trigger.cpp \
	game/Vr.cpp \
	game/Weapon.cpp \
	game/WorldSpawn.cpp \
	#ai/AAS_NearPoint.cpp \
    #game/Prey/ai_Navigator.cpp \

LOCAL_SRC_FILES = $(src_idlib) $(src_game)

LOCAL_SHARED_LIBRARIES := 
LOCAL_STATIC_LIBRARIES :=d3es_
LOCAL_LDLIBS := -landroid -llog

include $(BUILD_SHARED_LIBRARY)
