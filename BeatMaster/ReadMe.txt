========================================================================
    APPLICATION : BeatMaster Project Overview
    DEVELOPER   :    Shahed Abdol (c) 2015
========================================================================

BeatMaster is supposed to be a fun little top-down shooter.

BeatMaster is written using a software renderer, which should remove some
of the stringent hardware requirements imposed by many modern games.


The game is a top-down shooter, which tries hard to give the illusion of
being a 3D game by using an inaccurate shadow projection technique.

The controls are fairly simple, and it uses up quite a bit of CPU, but in
it's unoptimized state it already hits about 300 fps in Release builds.


/////////////////////////////////////////////////////////////////////////////

The aim of the game is simply to shoot enemies while avoiding enemy fire.
The mechanics are simple, but we are hoping to improve a lot on the control
scheme we are currently using. Collect weapons and power ups to get a 
temporary boost during play.

Power ups will be equipped when the player needs them so that they can be
saved for difficult bits.

/////////////////////////////////////////////////////////////////////////////

Right now I am still working on the rasterization techniques, as I am not a
graphics genius so this might take up the bulk of the development time.

The graphical elements are quite interesting and require Paint.net to open.
I wrote the plugin myself, but it's quite easy to duplicate -

The .graw file format is as follows:
First 4 bytes are width (treat it as 1 int)
Next 4 bytes are height (treat is as 1 int)

The rest of the file is [Width * Height * 4] bytes.
Each 4 byte sequence is in the ARGB format.

Paint.net will allow you to write your own plugin which can load/save these
files, or you could write your own conversion routines if you want.


/////////////////////////////////////////////////////////////////////////////

Ideally, I am trying to keep the game + resources as small as possible which
comes at the expense of neatness/resolution so even though it looks 
pixellated, rest assured that this is by design :)

TODO:

Implement power ups.
Implement smarter enemies.
Add sound.
Compress game data into an archive format of some sort.
Add more levels.

/////////////////////////////////////////////////////////////////////////////
