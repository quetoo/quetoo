/*
 * Copyright(c) 1997-2001 id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quetoo.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#pragma once

#include "files.h"
#include "filesystem.h"
#include "monitor.h"
#include "thread.h"
#include "collision/cm_material.h"

int32_t BSP_Main(void);
int32_t VIS_Main(void);
int32_t LIGHT_Main(void);
int32_t AAS_Main(void);
int32_t MAT_Main(void);
int32_t ZIP_Main(void);

extern char map_base[MAX_QPATH];

extern char map_name[MAX_OS_PATH];
extern char bsp_name[MAX_OS_PATH];

extern _Bool verbose;
extern _Bool debug;
extern _Bool legacy;

// VIS
extern _Bool fastvis;
extern _Bool nosort;

// BSP
extern int32_t entity_num;

extern vec3_t map_mins, map_maxs;

extern _Bool noprune;
extern _Bool nodetail;
extern _Bool fulldetail;
extern _Bool onlyents;
extern _Bool nomerge;
extern _Bool nowater;
extern _Bool nocsg;
extern _Bool noweld;
extern _Bool noshare;
extern _Bool nosubdivide;
extern _Bool notjunc;
extern _Bool noopt;
extern _Bool leaktest;

extern int32_t block_xl, block_xh, block_yl, block_yh;
extern int32_t subdivide_size;

extern vec_t microvolume;

// LIGHT
extern _Bool antialias;
extern _Bool indirect;

extern vec_t brightness;
extern vec_t saturation;
extern vec_t contrast;

extern vec_t surface_scale;
extern vec_t entity_scale;

extern vec3_t ambient;

extern vec_t patch_subdivide;

// threads.c
typedef struct semaphores_s {
	SDL_sem *active_portals;
	SDL_sem *active_nodes;
	SDL_sem *vis_nodes;
	SDL_sem *nonvis_nodes;
	SDL_sem *active_brushes;
	SDL_sem *active_windings;
	SDL_sem *removed_points;
} semaphores_t;

extern semaphores_t semaphores;

void Sem_Init(void);
void Sem_Shutdown(void);

typedef struct thread_work_s {
	int32_t index; // current work cycle
	int32_t count; // total work cycles
	int32_t fraction; // last fraction of work completed
	_Bool progress; // are we reporting progress
} thread_work_t;

extern thread_work_t thread_work;

typedef void (*ThreadWorkFunc)(int32_t);

void ThreadLock(void);
void ThreadUnlock(void);
void RunThreadsOn(int32_t workcount, _Bool progress, ThreadWorkFunc func);

enum {
	MEM_TAG_QUEMAP	= 1000,
	MEM_TAG_TREE,
	MEM_TAG_NODE,
	MEM_TAG_BRUSH,
	MEM_TAG_EPAIR,
	MEM_TAG_FACE,
	MEM_TAG_VIS,
	MEM_TAG_LIGHT,
	MEM_TAG_FACE_LIGHTING,
	MEM_TAG_PATCH,
	MEM_TAG_WINDING,
	MEM_TAG_PORTAL,
	MEM_TAG_ASSET
};
