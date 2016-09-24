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

#include "g_local.h"
#include "bg_pmove.h"

/**
 * @brief
 */
static void G_misc_teleporter_Touch(g_entity_t *self, g_entity_t *other,
		const cm_bsp_plane_t *plane __attribute__((unused)),
		const cm_bsp_surface_t *surf __attribute__((unused))) {

	if (!G_IsMeat(other))
		return;

	const g_entity_t *dest = G_Find(NULL, LOFS(target_name), self->locals.target);

	if (!dest) {
		gi.Warn("Couldn't find destination\n");
		return;
	}

	// unlink to make sure it can't possibly interfere with G_KillBox
	gi.UnlinkEntity(other);

	VectorCopy(dest->s.origin, other->s.origin);
	other->s.origin[2] += 8.0;

	vec3_t forward;
	AngleVectors(dest->s.angles, forward, NULL, NULL);

	if (other->client) {
		// overwrite velocity and hold them in place briefly
		other->client->ps.pm_state.flags &= ~PMF_TIME_MASK;
		other->client->ps.pm_state.flags = PMF_TIME_TELEPORT;

		other->client->ps.pm_state.time = 20;

		// set delta angles
		vec3_t delta_angles;
		VectorSubtract(dest->s.angles, other->client->locals.cmd_angles, delta_angles);
		PackAngles(delta_angles, other->client->ps.pm_state.delta_angles);

		VectorScale(forward, other->client->locals.speed, other->locals.velocity);

		VectorClear(other->client->locals.cmd_angles);
		VectorClear(other->client->locals.angles);
	} else {

		const vec3_t vel = { other->locals.velocity[0], other->locals.velocity[1], 0.0 };
		VectorScale(forward, VectorLength(vel), other->locals.velocity);

		other->s.angles[1] += dest->s.angles[1];
	}

	other->locals.velocity[2] = 150.0;

	// draw the teleport effect at source and dest
	self->s.event = EV_CLIENT_TELEPORT;
	other->s.event = EV_CLIENT_TELEPORT;

	VectorClear(other->s.angles);

	G_KillBox(other); // telefrag anyone in our spot

	gi.LinkEntity(other);
}

/*QUAKED misc_teleporter (1 0 0) (-32 -32 -24) (32 32 -16) ? ? no_effects
 Warps players who touch this entity to the targeted misc_teleporter_dest entity.

 -------- Spawn flags --------
 ?
 ?
 no_effects : Suppress the default teleporter particle effects.
 */
void G_misc_teleporter(g_entity_t *ent) {
	vec3_t v;

	if (!ent->locals.target) {
		gi.Debug("No target specified\n");
		G_FreeEntity(ent);
		return;
	}

	ent->solid = SOLID_TRIGGER;
	ent->locals.move_type = MOVE_TYPE_NONE;

	if (ent->model) { // model form, trigger_teleporter
		gi.SetModel(ent, ent->model);
		ent->sv_flags = SVF_NO_CLIENT;
	} else { // or model-less form, misc_teleporter
		VectorSet(ent->mins, -32.0, -32.0, -24.0);
		VectorSet(ent->maxs, 32.0, 32.0, -16.0);

		VectorCopy(ent->s.origin, v);
		v[2] -= 16.0;

		// add effect if ent is not buried and effect is not inhibited
		if (!gi.PointContents(v) && !(ent->locals.spawn_flags & 4)) {
			ent->s.sound = gi.SoundIndex("world/teleport_hum");
			ent->s.trail = TRAIL_TELEPORTER;
		}
	}

	ent->locals.Touch = G_misc_teleporter_Touch;

	gi.LinkEntity(ent);
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
 Teleport destination for misc_teleporters.

 -------- Keys --------
 angle : Direction in which player will look when teleported.
 targetname : The target name of this entity.
 */
void G_misc_teleporter_dest(g_entity_t *ent) {
	G_InitPlayerSpawn(ent);
}

/**
 * @brief
 */
static void G_misc_fireball_Think(g_entity_t *self) {

	if (self->locals.ground_entity) {
		self->solid = SOLID_NOT;

		self->s.effects = EF_DESPAWN;

		self->locals.move_type = MOVE_TYPE_NO_CLIP;
		self->locals.velocity[2] = -8.0;

		self->locals.Think = G_FreeEntity;
		self->locals.next_think = g_level.time + 3000;

		gi.LinkEntity(self);
	} else {
		G_FreeEntity(self);
	}
}

/**
 * @brief
 */
static void G_misc_fireball_Touch(g_entity_t *self, g_entity_t *other,
		const cm_bsp_plane_t *plane __attribute__((unused)),
		const cm_bsp_surface_t *surf __attribute__((unused))) {

	if (g_level.time - self->locals.touch_time > 500) {
		self->locals.touch_time = g_level.time;

		G_Damage(other, self, NULL, self->locals.velocity, self->s.origin, NULL, self->locals.damage, self->locals.damage, 0, MOD_FIREBALL);
	}
}

/**
 * @brief
 */
static void G_misc_fireball_Fly(g_entity_t *self) {
	static uint32_t count;

	g_entity_t *ent = G_AllocEntity(__func__);

	VectorCopy(self->s.origin, ent->s.origin);

	VectorSet(ent->mins, -3.0, -3.0, -3.0);
	VectorSet(ent->maxs, 3.0, 3.0, 3.0);

	AngleVectors(self->s.angles, ent->locals.velocity, NULL, NULL);
	VectorScale(ent->locals.velocity, self->locals.speed, ent->locals.velocity);

	for (int32_t i = 0; i < 3; i++) {
		ent->locals.velocity[i] += Randomc() * 30.0;
	}

	VectorSet(ent->locals.avelocity, Randomc() * 10.0, Randomc() * 10.0, Randomc() * 20.0);

	ent->s.trail = TRAIL_FIREBALL;

	ent->solid = SOLID_TRIGGER;
	ent->locals.move_type = MOVE_TYPE_BOUNCE;

	gi.SetModel(ent, "models/gibs/gib_1/tris");
	ent->locals.damage = self->locals.damage;

	ent->locals.Touch = G_misc_fireball_Touch;

	ent->locals.Think = G_misc_fireball_Think;
	ent->locals.next_think = g_level.time + 3000;

	gi.LinkEntity(ent);

	if (Randomf() < 0.33) {
		gi.Sound(ent, gi.SoundIndex(va("world/lava_%d", (count++ % 3) + 1)), ATTEN_IDLE);
	}

	self->locals.next_think = g_level.time + (self->locals.wait * 1000.0) + (self->locals.random * Randomc() * 1000);
}

/*QUAKED misc_fireball (1 0.3 0.1) (-6 -6 -6) (6 6 6)
 Spawns an intermittent fireball that damages players. These are typically used above lava traps for ambiance.

 -------- Keys --------
 angles : The angles at which the fireball will fly (default randomized).
 dmg : The damage inflicted to entities that touch the fireball (default 4).
 random : Random time variance in seconds added to "wait" delay (default 0.5 * wait).
 speed : The speed at which the fireball will fly (default 600.0).
 wait : The interval in seconds between fireball emissions (default 5.0).
 */
void G_misc_fireball(g_entity_t *self) {

	for (int32_t i = 1; i < 4; i++) {
		gi.SoundIndex(va("world/lava_%d", i));
	}

	if (VectorCompare(self->s.angles, vec3_origin)) {
		VectorSet(self->s.angles, -90.0, 0.0, 0.0);
	}

	if (self->locals.damage == 0) {
		self->locals.damage = 4;
	}

	if (self->locals.speed == 0.0) {
		self->locals.speed = 600.0;
	}

	if (self->locals.wait == 0.0) {
		self->locals.wait = 5.0;
	}

	if (self->locals.random == 0.0) {
		self->locals.random = self->locals.wait * 0.5;
	}

	self->locals.Think = G_misc_fireball_Fly;
	self->locals.next_think = g_level.time + (Randomf() * 1000);
}

