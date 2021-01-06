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

#include "r_local.h"

static int32_t stain_frame;

/**
 * @brief Attempt to stain the surface.
 */
static void R_StainFace(const r_stain_t *stain, r_bsp_face_t *face) {

	vec3_t point;
	Matrix4x4_Transform(&face->lightmap.matrix, stain->origin.xyz, point.xyz);

	vec2_t st = Vec2_Subtract(Vec3_XY(point), face->lightmap.st_mins);

	const vec2_t size = Vec2_Subtract(face->lightmap.st_maxs, face->lightmap.st_mins);
	const vec2_t padding = Vec2_Subtract(Vec2(face->lightmap.w, face->lightmap.h), size);

	st = Vec2_Add(st, Vec2_Scale(padding, .5f));

	// convert the radius to luxels
	const float radius = stain->radius / r_world_model->bsp->luxel_size;

	// square it to avoid a sqrt per luxe;
	const float radius_squared = radius * radius;

	// now iterate the luxels within the radius and stain them
	for (int32_t i = -radius; i <= radius; i++) {

		const ptrdiff_t t = st.y + i;
		if (t < 0 || t >= face->lightmap.h) {
			continue;
		}

		for (int32_t j = -radius; j <= radius; j++) {

			const ptrdiff_t s = st.x + j;
			if (s < 0 || s >= face->lightmap.w) {
				continue;
			}

			// this luxel is stained, so attenuate and blend it

			if (stain->shadow) {
				byte *shadowmap = face->lightmap.shadowmap + (face->lightmap.w * t + s);

				*shadowmap = (byte) (stain->color.r * 255.f);

				face->shadow_frame = stain_frame;
			} else {
				byte *stainmap = face->lightmap.stainmap + (face->lightmap.w * t + s) * BSP_LIGHTMAP_BPP;

				const float dist_squared = Vec2_LengthSquared(Vec2(i, j));
				const float atten = (radius_squared - dist_squared) / radius_squared;

				const float intensity = stain->color.a * atten * r_stains->value;

				const float src_alpha = Clampf(intensity, 0.0, 1.0);
				const float dst_alpha = 1.0 - src_alpha;

				const color_t src = Color_Scale(stain->color, src_alpha);
				const color_t dst = Color_Scale(Color3b(stainmap[0], stainmap[1], stainmap[2]), dst_alpha);

				const color32_t out = Color_Color32(Color_Add(src, dst));

				stainmap[0] = out.r;
				stainmap[1] = out.g;
				stainmap[2] = out.b;

				face->stain_frame = stain_frame;
			}
		}
	}
}

/**
 * @brief
 */
static void R_StainNode(const r_stain_t *stain, const r_bsp_node_t *node) {

	if (node->contents != CONTENTS_NODE) {
		return;
	}

	const cm_bsp_plane_t *plane = node->plane->cm;
	const float dist = Cm_DistanceToPlane(stain->origin, plane);

	if (dist > stain->radius) {
		R_StainNode(stain, node->children[0]);
		return;
	}

	if (dist < -stain->radius) {
		R_StainNode(stain, node->children[1]);
		return;
	}

	// project the stain onto the node's plane
	const r_stain_t s = {
		.origin = Vec3_Add(stain->origin, Vec3_Scale(plane->normal, -dist)),
		.radius = stain->radius - fabsf(dist),
		.color = stain->color,
		.shadow = stain->shadow
	};

	const int32_t side = dist > 0.f ? 0 : 1;

	r_bsp_face_t *face = node->faces;
	for (int32_t i = 0; i < node->num_faces; i++, face++) {

		if (face->plane_side != side) {
			continue;
		}

		if (face->texinfo->flags & SURF_MASK_NO_LIGHTMAP) {
			continue;
		}

		if (face->contents & SURF_LIQUID) {
			continue;
		}

		R_StainFace(&s, face);
	}

	// recurse down both sides
	R_StainNode(stain, node->children[0]);
	R_StainNode(stain, node->children[1]);
}

/**
 * @brief
 */
void R_AddStain(r_view_t *view, const r_stain_t *stain) {

	if (view->num_stains == MAX_STAINS) {
		Com_Warn("MAX_STAINS\n");
		return;
	}

	view->stains[view->num_stains++] = *stain;
}

/**
 * @brief
 */
static void R_ProjectStain(const r_view_t *view, const r_stain_t *stain) {

	if (!r_shadows->integer) {
		return;
	}

	const vec3_t down = Vec3_Down();
	const vec3_t right = Vec3(1.f, 0.f, 0.f);
	const vec3_t up = Vec3(0.f, 1.f, 0.f);

	const float scale = Maxf(1.f, (1 << (4 - r_shadows->integer)));
	const float fade_distance = 512.f;
	float shadow_alpha = Vec3_Distance(stain->origin, view->origin) / fade_distance;

	if (shadow_alpha >= 1) {
		return;
	} else if (shadow_alpha > 0.8f) {
		shadow_alpha = (shadow_alpha - 0.8f) / 0.2f;
	} else {
		shadow_alpha = 0.f;
	}

	shadow_alpha = 1.0f - shadow_alpha;
	
	const float w = stain->width * 0.5f;
	const float h = stain->height * 0.5f;

	for (float x = -w; x <= w; x += scale)
		for (float y = -h; y <= h; y += scale)
		{
			if (x * x * h * h + y * y * w * w > h * h * w * w) {
				continue;
			}

			vec3_t pt = stain->origin;
			pt = Vec3_Fmaf(pt, x, right);
			pt = Vec3_Fmaf(pt, y, up);

			cm_trace_t tr = Cm_BoxTrace(pt, Vec3_Fmaf(pt, MAX_WORLD_COORD, down), Vec3_Zero(), Vec3_Zero(), 0, CONTENTS_MASK_SOLID);

			if (tr.fraction == 1.0f) {
				continue;
			}

			float alpha = Vec3_Distance(pt, tr.end) / fade_distance;

			if (alpha >= 1) {
				continue;
			}

			alpha = (1.0f - alpha) * shadow_alpha;

			R_StainNode(&(const r_stain_t) {
				.origin = tr.end,
				.radius = scale,
				.color = Color3f(alpha, 0.f, 0.f),
				.shadow = true
			}, r_world_model->bsp->nodes);
		}
}

/**
 * @brief
 */
void R_UpdateStains(const r_view_t *view) {
	
	if (!view->num_stains) {
		return;
	}

	if (r_world_model->bsp->lightmap == NULL) {
		return;
	}

	stain_frame++;

	const r_stain_t *stain = view->stains;
	for (int32_t i = 0; i < view->num_stains; i++, stain++) {

		if (stain->projected && r_shadows->integer) {
			R_ProjectStain(view, stain);
		} else {
			R_StainNode(stain, r_world_model->bsp->nodes);

			const r_entity_t *e = view->entities;
			for (int32_t j = 0; j < view->num_entities; j++, e++) {
				if (e->model && e->model->type == MOD_BSP_INLINE) {

					r_stain_t s = *stain;
					Matrix4x4_Transform(&e->inverse_matrix, s.origin.xyz, s.origin.xyz);

					R_StainNode(&s, e->model->bsp_inline->head_node);
				}
			}
		}
	}

	const r_bsp_model_t *bsp = r_world_model->bsp;
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, bsp->lightmap->atlas->texnum);
	glBindTexture(GL_TEXTURE_2D, bsp->lightmap->shadowmap->texnum);

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, bsp->lightmap->shadowmap->width, bsp->lightmap->shadowmap->height, GL_RED, GL_UNSIGNED_BYTE, bsp->lightmap->empty_shadowmap);

	const r_bsp_face_t *face = bsp->faces;
	for (int32_t i = 0; i < bsp->num_faces; i++, face++) {

		if (face->stain_frame == stain_frame) {

			glTexSubImage3D(GL_TEXTURE_2D_ARRAY,
					0,
					face->lightmap.s,
					face->lightmap.t,
					BSP_LIGHTMAP_LAYERS,
					face->lightmap.w,
					face->lightmap.h,
					1,
					GL_RGB,
					GL_UNSIGNED_BYTE,
					face->lightmap.stainmap);
		}

		if (face->shadow_frame == stain_frame && r_shadows->integer) {

			glTexSubImage2D(GL_TEXTURE_2D,
					0,
					face->lightmap.s,
					face->lightmap.t,
					face->lightmap.w,
					face->lightmap.h,
					GL_RED,
					GL_UNSIGNED_BYTE,
					face->lightmap.shadowmap);

			memset(face->lightmap.shadowmap, 0, face->lightmap.w * face->lightmap.h);
		}
	}
	
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	R_GetError(NULL);
}
