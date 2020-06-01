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
#include "game/default/bg_pmove.h"
#include "collision/collision.h"

static vec3_t Cg_FibonacciLatticeDir(int32_t count, int32_t index) {
	// source: http://extremelearning.com.au/evenly-distributing-points-on-a-sphere/
	float phi = acosf(1.f - 2.f * index / count);
	float golden_ratio = (1.f + pow(5.f, .5f)) * .5f;
	float theta = 2.f * M_PI * index / golden_ratio;
	return Vec3(cosf(theta) * sinf(phi), sinf(theta) * sinf(phi), cosf(phi));
}

/**
 * @brief
 */
static void Cg_ItemRespawnEffect(const vec3_t org, const color_t color) {

	cg_sprite_t *s;

	int32_t particle_count = 256;
	float z_offset = 20.f;

	// sphere particles
	for (int32_t i = 0; i < particle_count; i++) {

		if (!(s = Cg_AllocSprite())) {
			break;
		}

		s->atlas_image = cg_sprite_particle2;

		s->lifetime = 1000.f;
		s->origin = org;
		s->origin.z += z_offset;
		s->velocity = Vec3_Scale(Cg_FibonacciLatticeDir(particle_count, i + 1), 55.f);
		s->acceleration = Vec3_Scale(s->velocity, -1.f);

		s->color = Color4f(1.f, 1.f, 1.f, 0.f);
		s->end_color = Color4f(.1f, .6f, .3f, 0.f);
		s->size = 5.f;
		s->size_velocity = -s->size / MILLIS_TO_SECONDS(s->lifetime);
	}

	// glow
	if ((s = Cg_AllocSprite())) {
		s->origin = org;
		s->origin.z += z_offset;
		s->lifetime = 1000;
		s->size = 200.f;
		s->atlas_image = cg_sprite_particle;
		s->color = Color4f(1.f, 1.f, 1.f, 0.f);
	}

	Cg_AddLight(&(cg_light_t) {
		.origin = org,
		.radius = 80.f,
		.color = Vec3(.1f, .6f, .3f),
		.decay = 1000
	});
}

/**
 * @brief
 */
static void Cg_ItemPickupEffect(const vec3_t org, const color_t color) {

	cg_sprite_t *s;
	// float z_offset = 20.f;

	// ring
	if ((s = Cg_AllocSprite())) {

		s->lifetime = 400;
		s->origin = org;
		// s->origin.z += z_offset;
		s->size = 10.f;
		s->size_velocity = 50.f / MILLIS_TO_SECONDS(s->lifetime);
		s->atlas_image = cg_sprite_ring;
		s->color = Color4f(.2f, .4f, .3f, 0.f);
		s->dir = Vec3(0.f, 0.f, 1.f);

	}

	// glow
	if ((s = Cg_AllocSprite())) {
		s->origin = org;
		// s->origin.z += z_offset;
		s->lifetime = 1000;
		s->size = 200.f;
		s->atlas_image = cg_sprite_particle;
		s->color = Color4f(.2f, .4f, .3f, 0.f);
	}

	Cg_AddLight(&(cg_light_t) {
		.origin = org,
		.radius = 80.f,
		.color = Vec3(.2f, .4f, .3f),
		.decay = 1000
	});
}

/**
 * @brief
 */
static void Cg_TeleporterEffect(const vec3_t org) {

	for (int32_t i = 0; i < 64; i++) {
		cg_sprite_t *s;

		if (!(s = Cg_AllocSprite())) {
			break;
		}

		s->atlas_image = cg_sprite_particle;
		s->size = 8.f;
		s->origin = Vec3_Add(org, Vec3_RandomRange(-16.f, 16.f));
		s->origin.z += RandomRangef(8.f, 32.f);
		s->velocity = Vec3_RandomRange(-24.f, 24.f);
		s->velocity.z = RandomRangef(16.f, 48.f);
		s->acceleration.z = -SPRITE_GRAVITY * 0.1;
		s->lifetime = 500;
		s->color = Color3b(224, 224, 224);
		s->size = 8.f;
	}

	Cg_AddLight(&(cg_light_t) {
		.origin = org,
		.radius = 120.f,
		.color = Vec3(.9f, .9f, .9f),
		.decay = 1000
	});

	const s_play_sample_t play = {
		.sample = cg_sample_respawn,
		.origin = org,
		.attenuation = ATTEN_IDLE,
		.flags = S_PLAY_POSITIONED
	};

	cgi.AddSample(&play);
}

/**
 * @brief A player is gasping for air under water.
 */
static void Cg_GurpEffect(cl_entity_t *ent) {

	const s_play_sample_t play = {
		.sample = cgi.LoadSample("*gurp_1"),
		.entity = ent->current.number,
		.attenuation = ATTEN_NORM,
		.flags = S_PLAY_ENTITY
	};

	cgi.AddSample(&play);

	vec3_t start = ent->current.origin;
	start.z += 16.0;

	vec3_t end = start;
	end.z += 16.0;

	Cg_BubbleTrail(NULL, start, end, 32.0);
}

/**
 * @brief A player has drowned.
 */
static void Cg_DrownEffect(cl_entity_t *ent) {

	const s_play_sample_t play = {
		.sample = cgi.LoadSample("*drown_1"),
		.entity = ent->current.number,
		.attenuation = ATTEN_NORM,
		.flags = S_PLAY_ENTITY
	};

	cgi.AddSample(&play);

	vec3_t start = ent->current.origin;
	start.z += 16.0;

	vec3_t end = start;
	end.z += 16.0;

	Cg_BubbleTrail(NULL, start, end, 32.0);
}

/**
 * @brief
 */
static s_sample_t *Cg_Footstep(cl_entity_t *ent) {

	const char *footsteps = "default";

	vec3_t start = ent->current.origin;
	start.z += ent->current.mins.z;

	vec3_t end = start;
	end.z -= PM_STEP_HEIGHT;

	cm_trace_t tr = cgi.Trace(start, end, Vec3_Zero(), Vec3_Zero(), ent->current.number, CONTENTS_MASK_SOLID);

	if (tr.fraction < 1.0 && tr.surface && tr.surface->material && *tr.surface->material->footsteps) {
		footsteps = tr.surface->material->footsteps;
	}

	return Cg_GetFootstepSample(footsteps);
}

/**
 * @brief Process any event set on the given entity. These are only valid for a single
 * frame, so we reset the event flag after processing it.
 */
void Cg_EntityEvent(cl_entity_t *ent) {

	entity_state_t *s = &ent->current;

	s_play_sample_t play = {
		.entity = s->number,
		.attenuation = ATTEN_NORM,
		.flags = S_PLAY_ENTITY,
	};

	switch (s->event) {
		case EV_CLIENT_DROWN:
			Cg_DrownEffect(ent);
			break;
		case EV_CLIENT_FALL:
			play.sample = cgi.LoadSample("*fall_2");
			break;
		case EV_CLIENT_FALL_FAR:
			play.sample = cgi.LoadSample("*fall_1");
			break;
		case EV_CLIENT_FOOTSTEP:
			play.sample = Cg_Footstep(ent);
			play.pitch = RandomRangei(-12, 13);
			break;
		case EV_CLIENT_GURP:
			Cg_GurpEffect(ent);
			break;
		case EV_CLIENT_JUMP:
			play.sample = cgi.LoadSample(va("*jump_%d", RandomRangeu(1, 5)));
			break;
		case EV_CLIENT_LAND:
			play.sample = cgi.LoadSample("*land_1");
			break;
		case EV_CLIENT_STEP: {
			const float height = ent->current.origin.z - ent->prev.origin.z;
			Cg_TraverseStep(&ent->step, cgi.client->unclamped_time, height);
		}
			break;
		case EV_CLIENT_SIZZLE:
			play.sample = cgi.LoadSample("*sizzle_1");
			break;
		case EV_CLIENT_TELEPORT:
			play.sample = cg_sample_teleport;
			play.attenuation = ATTEN_IDLE;
			Cg_TeleporterEffect(s->origin);
			break;

		case EV_ITEM_RESPAWN:
			play.sample = cg_sample_respawn;
			play.attenuation = ATTEN_IDLE;
			Cg_ItemRespawnEffect(s->origin, color_white); //TODO: wire up colors, white is placeholder
			break;
		case EV_ITEM_PICKUP:
			Cg_ItemPickupEffect(s->origin, color_white); // TODO: wire up colors, white is placeholder
			break;

		default:
			break;
	}

	if (play.sample) {
		cgi.AddSample(&play);
	}

	s->event = 0;
}
