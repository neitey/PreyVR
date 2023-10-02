[![PreyVR Banner](https://github.com/lvonasek/Doom3Quest/blob/master/app/src/main/res/drawable/ic_launcher.png?raw=true)](https://www.deviantart.com/madrapper/art/Prey-Icon-129814229)

# Introduction

*From [Wikipedia](https://en.wikipedia.org/wiki/Prey_(2006_video_game)), the free encyclopedia*:

> Prey is a first-person shooter video game developed by Human Head Studios, under contract for 3D Realms, and published by 2K Games, while the Xbox 360 version was ported by Venom Games. The game was initially released in Noro th America and Europe on July 11, 2006. Prey uses a heavily modified version of id Tech 4 to use portals and variable gravity to create the environments the player explores.

> The game's story is focused on Cherokee Domasi "Tommy" Tawodi as he, his girlfriend, and grandfather are abducted aboard an alien spaceship known as The Sphere as it consumes material, both inanimate and living, from Earth in order to sustain itself. Tommy's Cherokee heritage allows him to let his spirit roam freely at times and come back to life after dying, which gives Tommy an edge in his battle against the Sphere.

> Prey had been in development in one form or another since 1995, and has had several major revisions. While the general approach to gameplay, including the use of portals, remained in the game, the story and setting changed several times. The game received generally positive reviews, with critics praising its graphics and gameplay but criticizing its multiplayer component for a lack of content.

> Prey was a commercial success, selling more than one million copies in the first two months of its release and leading to the abortive development of its sequel Prey 2. The rights to Prey passed on to Bethesda Softworks, who released an unrelated game of the same name, developed by Arkane Studios, in 2017.

Prey 2006 is a wild mashup of Portal, Doom 3, Half-Life and Twilight Zone.

In this repository I port the game on standalone VR headsets.

### Currently supported headsets:
* Meta/Oculus Quest 1, Quest 2, Quest Pro

### Planned support for headsets:
* Meta/Oculus Quest 3
* Pico Neo 3, Pico 4

# Instructions

### Demo version

* Download the game from [releases](https://github.com/lvonasek/PreyVR/releases).
* Install both APKs to your headset e.g. using SideQuest.
* In the headset open the game, it will download the demo data.

### Full version

Running the full version of this game require a copy of the original Prey 2006 data patched with 1.4 patch:

* Install the APK from github via Sidequest and open it once on your headset. It will close down. It has created the folders `preyvr/preybase`.
* Go to your steam installation of Prey. Navigate to `prey/base` on your PC. Copy all PK4 files from there to `preyvr/preybase` on your headset.
* Prey (2006) is delisted on Steam. But you can buy and redeem a steam key from a key reseller of your choice just fine.

[Click here to watch Gamertag's video tutorial](https://www.youtube.com/watch?v=OPXp0RYOSoA&t=542s)

# About the project

PreyVR is based on a combination of the excellent projects created by opensource community.
I tried to create a kind of family tree to keep track of the project's roots.

![Project family tree](https://github.com/lvonasek/PreyVR/blob/master/doc/family_tree.png?raw=true)

PreyVR uses renderer and VR integration from Doom3Quest, the engine is from glKarin's Android port. The project uses OpenXR to be easily adaptable to any Android based VR platform. The exception are Meta Quest headsets which have on OpenXR degraded performance. For Quest the releases are deployed from VrAPI branch.

### Contributors
* Engine consulting: **DrBeef**, **Baggyg**, **Bummser**, **Stiefl525**
* Programming: **Luboš**, **GLKarin**
* SideQuest listing: **Bummser**
* Weapons 3D modeling: **Luboš**, **LennyGuy20**

Special thanks to all early versions testers on **Team Beef** discord server!

### Bug fixes
* Alien text translations logic
* Credits rendering fixed (workaround for unsupported UI feature)
* Light beams culling fixed
* Performance issues when rendering skybox
* Rendered beams face camera
* Renderer fixes for specific materials/objects
* Z-Fighting for fire decals on walls

### Disabled features
* Developer console (demo data / mods issues)
* EAX reverb in dreamworld levels (this doesn't work well when invaded)
* Footsteps for 6DoF movements (it didn't apply to real steps)
* Knockback of leech gun (doesn't work correctly on higher framerate)
* Shadows rendering (causes performance issues)
* Stereo rendering for skyboxes (had wrong stereo effect)
* View bobbing (feels weird in VR)

### Features added to this project
* "3D" aiming cursor with autoscaling
* 6DoF motion tracking (mapping movement to keys + elevation adjust)
* 6DoF weapon tracking (apply controller movement to weapons)
* Apply headset refreshrate to game
* Automatic fix pf axis misalignment
* Big hunter scene recreated (the engine doesn't support original animation)
* Camera shaking option (shaking adds immersion but also motion sickness)
* Demo data without DDS textures (DDS textures are unsupported by the engine)
* Downloader to get demo data or mods
* Downscale HUD graphics (to be visible for VR users)
* Faster vertex cache
* Fluent rifle weapon zooming
* Full weapons models (original game doesn't contain back side of the weapon)
* Height adjust (helpful for sitting mode)
* Hidden god mode cheat trick
* Menu screens adjusted (workaround for missing UI features, VR options)
* Motion attack for wrench weapon
* Motion sickness warning on level 2 loading screen
* Opening ammo cabinets (missing in the Android version)
* Overlay effects adjusted to VR screen
* Stereo rendering
* Switching between VR/flat mode depending on scene
* VR vehicle control

### Known issues
* 6DoF doesn't allow movement over invisible colliders
* Alien light effects in the first level are not correct
* Jen has detached head on screen (level Hidden Agendas)
* Jukebox doesn't update the pointer (collider issue)
* Performance issues when fog is present
* Performance issues when portals are shown
* Screen with Jen is not rendered (level Hidden Agendas)
* UI tabs are not rendered correctly
* Weapons have incorrect alignment when putting them down/behind
  
### Missing features
* BHaptics support (I do not own the accessory)
* Controling ingame UI using fingers
* Glitch effect (unsupported by the renderer) 
* Glow effect (unsupported by the renderer)
* Network communication (missing source code)
* Shadows (cause performance issues)
