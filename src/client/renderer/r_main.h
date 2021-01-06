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

#include "r_types.h"

extern cvar_t *r_get_error;

extern cvar_t *r_allow_high_dpi;
extern cvar_t *r_anisotropy;
extern cvar_t *r_brightness;
extern cvar_t *r_bicubic;
extern cvar_t *r_caustics;
extern cvar_t *r_contrast;
extern cvar_t *r_display;
extern cvar_t *r_flares;
extern cvar_t *r_fog_density;
extern cvar_t *r_fog_samples;
extern cvar_t *r_fullscreen;
extern cvar_t *r_gamma;
extern cvar_t *r_hardness;
extern cvar_t *r_height;
extern cvar_t *r_modulate;
extern cvar_t *r_multisample;
extern cvar_t *r_parallax;
extern cvar_t *r_parallax_samples;
extern cvar_t *r_roughness;
extern cvar_t *r_saturation;
extern cvar_t *r_screenshot_format;
extern cvar_t *r_shadows;
extern cvar_t *r_shell;
extern cvar_t *r_specularity;
extern cvar_t *r_stains;
extern cvar_t *r_texture_mode;
extern cvar_t *r_swap_interval;
extern cvar_t *r_width;

extern r_stats_t r_stats;

void R_GetError_(const char *function, const char *msg);
#define R_GetError(msg) R_GetError_(__func__, msg)

void R_Init(void);
void R_Shutdown(void);
void R_BeginFrame(r_view_t *view);
void R_DrawViewDepth(r_view_t *view);
void R_DrawView(r_view_t *view);
void R_EndFrame(void);

#ifdef __R_LOCAL_H__

/**
 * @brief OpenGL driver information.
 */
typedef struct {
	const char *renderer;
	const char *vendor;
	const char *version;

	int32_t max_texunits;
	int32_t max_texture_size;
} r_config_t;

extern r_config_t r_config;

/**
 * @brief The lightgrid uniform struct.
 * @remarks This struct is vec4 aligned.
 */
typedef struct {
	/**
	 * @brief The lightgrid mins, in world space.
	 */
	vec4_t mins;

	/**
	 * @brief The lightgrid maxs, in world space.
	 */
	vec4_t maxs;

	/**
	 * @brief The view origin, in lightgrid space.
	 */
	vec4_t view_coordinate;

	/**
	 * @brief The lightgrid texel dimensions.
	 */
	vec4_t resolution;
} r_lightgrid_t;

/**
 * @brief The uniforms block type.
 */
typedef struct {
	/**
	 * @brief The name of the uniform buffer.
	 */
	GLuint buffer;

	/**
	 * @brief The uniform block struct.
	 * @remarks This struct is vec4 aligned.
	 */
	struct {
		/**
		 * @brief The viewport (x, y, w, h) in device pixels.
		 */
		vec4_t viewport;

		/**
		 * @brief The 3D projection matrix.
		 */
		mat4_t projection3D;

		/**
		 * @brief The 2D projection matrix.
		 */
		mat4_t projection2D;

		/**
		 * @brief The 2D projection matrix for the framebuffer object.
		 */
		mat4_t projection2D_FBO;

		/**
		 * @brief The view matrix.
		 */
		mat4_t view;

		/**
		 * @brief The lightgrid uniforms.
		 */
		r_lightgrid_t lightgrid;

		/**
		 * @brief The depth range, in world units.
		 */
		vec2_t depth_range;

		/**
		 * @brief The renderer time, in milliseconds.
		 */
		int32_t ticks;

		/**
		 * @brief The brightness scalar.
		 */
		float brightness;

		/**
		 * @brief The contrast scalar.
		 */
		float contrast;

		/**
		 * @brief The saturation scalar.
		 */
		float saturation;

		/**
		 * @brief The gamma scalar.
		 */
		float gamma;

		/**
		 * @brief The modulate scalar.
		 */
		float modulate;

		/**
		 * @brief The global fog color.
		 */
		// vec3_t fog_global_color; // FIXME

		/**
		 * @brief The global fog density scalar.
		 */
		// float fog_global_density; // FIXME

		/**
		 * @brief The volumetric fog density scalar.
		 */
		float fog_density;

		/**
		 * @brief The number of volumetric fog samples per fragment (quality).
		 */
		int32_t fog_samples;

		/**
		* @brief The pixel dimensions of the framebuffer.
		*/
		vec2_t resolution;
	} block;

} r_uniforms_t;

/**
 * @brief The uniform variables block, updated once per frame and common to all programs.
 */
extern r_uniforms_t r_uniforms;

// developer tools
extern cvar_t *r_blend_depth_sorting;
extern cvar_t *r_clear;
extern cvar_t *r_cull;
extern cvar_t *r_depth_pass;
extern cvar_t *r_draw_bsp_lightgrid;
extern cvar_t *r_draw_bsp_normals;
extern cvar_t *r_draw_entity_bounds;
extern cvar_t *r_draw_material_stages;
extern cvar_t *r_draw_wireframe;
extern cvar_t *r_occlude;

_Bool R_CullPoint(const r_view_t *view, const vec3_t point);
_Bool R_CullBox(const r_view_t *view, const vec3_t mins, const vec3_t maxs);
_Bool R_CullSphere(const r_view_t *view, const vec3_t point, const float radius);

#endif /* __R_LOCAL_H__ */
