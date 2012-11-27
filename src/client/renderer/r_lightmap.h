/*
 * Copyright(c) 1997-2001 Id Software, Inc.
 * Copyright(c) 2002 The Quakeforge Project.
 * Copyright(c) 2006 Quake2World.
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

#ifndef __R_LIGHTMAP_H__
#define __R_LIGHTMAP_H__

#include "r_types.h"

#ifdef __R_LOCAL_H__
typedef struct r_lightmaps_s {
	GLuint lightmap_texnum;
	GLuint deluxemap_texnum;

	r_pixel_t block_size;  // lightmap block size (NxN)
	r_pixel_t *allocated;  // block availability

	byte *sample_buffer;  // RGB buffers for uploading
	byte *direction_buffer;

	float fbuffer[MAX_BSP_LIGHTMAP * 3];  // RGB buffer for bsp loading
} r_lightmaps_t;

extern r_lightmaps_t r_lightmaps;

void R_BeginBuildingLightmaps(r_bsp_model_t *bsp);
void R_CreateSurfaceLightmap(r_bsp_model_t *bsp, r_bsp_surface_t *surf);
void R_EndBuildingLightmaps(r_bsp_model_t *bsp);
#endif /* __R_LOCAL_H__ */

#endif /* __R_LIGHTMAP_H__ */

