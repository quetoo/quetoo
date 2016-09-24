[![Build Status](http://ci.quetoo.org/buildStatus/icon?job=Quetoo-Linux-x86_64)](http://ci.quetoo.org/job/Quetoo-Linux-x86_64/)
[![GPLv2 License](https://img.shields.io/badge/license-GPL%20v2-brightgreen.svg)](https://opensource.org/licenses/GPL-2.0)
![This software is BETA](https://img.shields.io/badge/development_stage-BETA-yellowgreen.svg)

# Quetoo BETA

![Quetoo BETA](https://scontent-mia1-1.xx.fbcdn.net/t31.0-8/861008_617224188323107_1551552667_o.jpg)

## Overview

[_Quetoo_](http://quetoo.org) is a Free first person shooter for Mac, PC and Linux. Our goal is to bring the fun of oldschool deathmatch to a new generation of gamers.

## Features

 * Deathmatch, Capture, Instagib, Duel and Rocket Arena gameplay modes
 * Teams Play and Match Mode support
 * High quality remakes of id Software's legendary _Quake II_ deathmatch levels, as well as originals
 * Original sounds and game music by <a href="http://rolandshaw.wordpress.com/">Roland Shaw</a> and <a href="http://anthonywebbmusic.com/">Anthony Webb</a>.
 * 100% free to download, play and modify. See our <a href="http://quetoo.org/books/documentation/licensing">licensing</a> page.

## Downloads

Preview releases of _Quetoo_ for all platforms are available for download on the [project website](http://quetoo.org/pages/downloads). Installation instructions are available there as well.

## Compiling

Compilation of _Quetoo_ is only recommended for users running GNU/Linux or Mac OS X. Windows users should consider using our cross-compiled snapshots. For more information, see [Installation and Maintenance](http://quetoo.org/books/documentation/installation-and-maintenance).

The following dependencies are required:

 * [SDL2_image](https://www.libsdl.org/projects/SDL_image/)
 * [SDL2_mixer](https://www.libsdl.org/projects/SDL_mixer/)
 * [PhysicsFS](https://icculus.org/physfs/)
 * [glib2](https://developer.gnome.org/glib/)
 * [cURL](https://curl.haxx.se/libcurl/)
 * [ncurses](https://www.gnu.org/software/ncurses/)
 * [libxml2](http://xmlsoft.org/)
 * [ObjectivelyMVC](https://github.com/jdolan/ObjectivelyMVC/)

### Build with autotools

To build it, run the following:

    autoreconf -i
    ./configure [--with-tests --with-master]
    make && sudo make install

### Build with Cmake

Options:

	-DCMAKE_BUILD_TYPE=[Release|Debug|RelWithDebInfo] (Default: RelWithDebInfo)
	-DCMAKE_INSTALL_PREFIX=[INSTALL_PATH]
	-DBUILD_TESTS=[On|Off] (Defeault: Off)

To build it with MSYS:

	mkdir build && cd build
    cmake -G "MSYS Makefiles" [OPTIONS] ..
    make && make install

To build it with MINGW:

	mkdir build && cd build
    cmake -G "MinGW Makefiles" [OPTIONS] ..
    make && make install

To build it with Cygwin and Linux:

	mkdir build && cd build
    cmake -G [OPTIONS] ..
    make && make install

## Installing

To have a working game, you must install the game data. You have two options: `git` or `rsync`. If you wish to work on or modify the game data, you should use `git`:

    git clone https://github.com/jdolan/quetoo-data.git
    sudo ln -s quetoo-data/target /usr/local/share/quetoo

If you have no plans to modify the game data, you can simply run `sudo make rsync-data` from the source code working copy.

More information on hacking on _Quetoo_ is available [on the project website](http://quetoo.org/books/documentation/developing-and-modding).

## Support
 * The IRC channel for this project is *#quetoo* on *irc.freenode.net*
