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

* Download the game and the launcher from [releases](https://github.com/lvonasek/PreyVR/releases).
* Install both APKs to your headset e.g. using SideQuest.
* In the headset open the launcher which will download the demo data.

### Full version

Running the full version of this game require a copy of the original Prey 2006 data patched with 1.4 patch:

* Install the APK from github via Sidequest and open it once on your headset. It will close down. It has created the folders `preyvr/preybase`.
* Go to your steam installation of Prey. Navigate to `prey/base` on your PC. Copy all PK4 files from there to `preyvr/preybase` on your headset.
* Prey (2006) is delisted on Steam. But you can buy and redeem a steam key from a key reseller of your choice just fine.

[Click here to watch Gamertag's video tutorial](https://www.youtube.com/watch?v=OPXp0RYOSoA&t=542s)

# Headset compatibility

The project uses OpenXR to be easily adaptable to any Android based VR platform. The exception are Meta Quest headsets which have on OpenXR degraded performance. For Quest the releases are deployed from VrAPI branch.

# Project family tree

PreyVR is based on a combination of the excellent projects created by opensource community.
I tried to create a kind of fmaily tree to keep track of the project's roots.

![Project family tree](https://github.com/lvonasek/PreyVR/blob/master/doc/family_tree.png?raw=true)
