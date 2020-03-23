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

#include "cg_local.h"

static cg_sprite_t *cg_free_sprites; // list of free sprites
static cg_sprite_t *cg_active_sprites; // list of active sprites

static cg_sprite_t cg_sprites[MAX_SPRITES];

/**
 * @brief Pushes the sprite onto the head of specified list.
 */
static void Cg_PushSprite(cg_sprite_t *p, cg_sprite_t **list) {

	p->prev = NULL;

	if (*list) {
		(*list)->prev = p;
	}

	p->next = *list;
	*list = p;
}

/**
 * @brief Pops the sprite from the specified list, repairing the list if it
 * becomes broken.
 */
static void Cg_PopSprite(cg_sprite_t *p, cg_sprite_t **list) {

	if (p->prev) {
		p->prev->next = p->next;
	}

	if (p->next) {
		p->next->prev = p->prev;
	}

	if (*list == p) {
		*list = p->next;
	}

	p->prev = p->next = NULL;
}

/**
 * @brief Allocates a free sprite with the specified type and image.
 */
cg_sprite_t *Cg_AllocSprite() {

	if (!cg_add_particles->integer) {
		return NULL;
	}

	if (!cg_free_sprites) {
		cgi.Debug("No free sprites\n");
		return NULL;
	}

	cg_sprite_t *p = cg_free_sprites;

	Cg_PopSprite(p, &cg_free_sprites);

	memset(p, 0, sizeof(cg_sprite_t));

	p->color = color_white;
	p->size = 1.0;

	p->time = p->timestamp = cgi.client->unclamped_time;

	Cg_PushSprite(p, &cg_active_sprites);

	return p;
}

/**
 * @brief Frees the specified sprite, returning the sprite it was pointing
 * to as a convenience for continued iteration.
 */
cg_sprite_t *Cg_FreeSprite(cg_sprite_t *p) {
	cg_sprite_t *next = p->next;

	Cg_PopSprite(p, &cg_active_sprites);

	Cg_PushSprite(p, &cg_free_sprites);

	return next;
}

/**
 * @brief Frees all sprites, returning them to the eligible list.
 */
void Cg_FreeSprites(void) {

	cg_free_sprites = NULL;
	cg_active_sprites = NULL;

	memset(cg_sprites, 0, sizeof(cg_sprites));

	for (size_t i = 0; i < lengthof(cg_sprites); i++) {
		Cg_PushSprite(&cg_sprites[i], &cg_free_sprites);
	}
}

/**
 * @brief Adds all sprites that are active for this frame to the view.
 */
void Cg_AddSprites(void) {

	if (!cg_add_particles->integer) {
		return;
	}

	const float delta = MILLIS_TO_SECONDS(cgi.client->frame_msec);

	cg_sprite_t *p = cg_active_sprites;
	while (p) {

		if (p->time != cgi.client->unclamped_time) {
			if (cgi.client->unclamped_time - p->time > p->lifetime) {
				p = Cg_FreeSprite(p);
				continue;
			}
		}

		p->velocity = Vec3_Add(p->velocity, Vec3_Scale(p->acceleration, delta));
		p->origin = Vec3_Add(p->origin, Vec3_Scale(p->velocity, delta));

		p->color_velocity = Vec4_Add(p->color_velocity, Vec4_Scale(p->color_acceleration, delta));
		p->color = Color4fv(Vec4_Add(Color_Vec4(p->color), Vec4_Scale(p->color_velocity, delta)));

		if (p->color.a <= 0) {
			p = Cg_FreeSprite(p);
			continue;
		}

		p->size_velocity += p->size_acceleration * delta;
		p->size += p->size_velocity * delta;

		if (p->size <= 0.f) {
			p = Cg_FreeSprite(p);
			continue;
		}

		switch (p->type) {
		case SPRITE_NORMAL:
			p->rotation += p->rotation_velocity * delta;

			cgi.AddSprite(&(r_sprite_t) {
				.origin = p->origin,
				.size = p->size,
				.color = p->color,
				.rotation = p->rotation,
				.image = p->image
			});
			break;
		case SPRITE_BEAM:
			p->beam.end = Vec3_Add(p->beam.end, Vec3_Scale(p->velocity, delta));

			cgi.AddBeam(&(r_beam_t) {
				.start = p->origin,
				.end = p->beam.end,
				.size = p->size,
				.image = p->image,
				.color = p->color
			});
			break;
		}
		
		p = p->next;
	}
}