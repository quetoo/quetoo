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
 * @see Pm_CheckGround
 */
static void G_CheckGround(g_entity_t *ent) {
	vec3_t pos;

	if (ent->locals.move_type == MOVE_TYPE_WALK)
		return;

	// check for ground interaction
	if (ent->locals.move_type == MOVE_TYPE_BOUNCE) {

		VectorCopy(ent->s.origin, pos);
		pos[2] -= PM_GROUND_DIST;
		
		// TODO: Use ent->locals.clip_mask?

		cm_trace_t trace = gi.Trace(ent->s.origin, pos, ent->mins, ent->maxs, ent, MASK_SOLID);

		if (trace.ent && trace.plane.normal[2] >= PM_STEP_NORMAL) {
			if (ent->locals.ground_entity == NULL) {
				gi.Debug("%s meeting ground %s\n", etos(ent), etos(trace.ent));
			}
			ent->locals.ground_entity = trace.ent;
			ent->locals.ground_plane = trace.plane;
			ent->locals.ground_surface = trace.surface;
			ent->locals.ground_contents = trace.contents;
		} else {
			if (ent->locals.ground_entity) {
				gi.Debug("%s leaving ground %s\n", etos(ent), etos(ent->locals.ground_entity));
			}
			ent->locals.ground_entity = NULL;
		}
	} else {
		ent->locals.ground_entity = NULL;
	}
}

static void G_CheckWater(g_entity_t *ent) {
	vec3_t pos, mins, maxs;
	
	if (ent->locals.move_type == MOVE_TYPE_WALK)
		return;

	// check for water interaction
	const uint8_t old_water_level = ent->locals.water_level;
	
	if (ent->solid == SOLID_BSP) {
		VectorLerp(ent->abs_mins, ent->abs_maxs, 0.5, pos);
		VectorSubtract(pos, ent->abs_mins, mins);
		VectorSubtract(ent->abs_maxs, pos, maxs);
	} else {
		VectorCopy(ent->s.origin, pos);
		VectorCopy(ent->mins, mins);
		VectorCopy(ent->maxs, maxs);
	}
	
	cm_trace_t tr = gi.Trace(pos, pos, mins, maxs, ent, MASK_LIQUID);
	
	ent->locals.water_type = tr.contents;
	ent->locals.water_level = ent->locals.water_type ? 1 : 0;

	if (!old_water_level && ent->locals.water_level) {
		gi.PositionedSound(pos, ent, g_media.sounds.water_in, ATTEN_IDLE);
		if (ent->locals.move_type == MOVE_TYPE_BOUNCE) {
			VectorScale(ent->locals.velocity, 0.66, ent->locals.velocity);
		}
	} else if (old_water_level && !ent->locals.water_level) {
		gi.PositionedSound(pos, ent, g_media.sounds.water_out, ATTEN_IDLE);
	}
}

/**
 * @brief Runs thinking code for this frame if necessary
 */
static void G_RunThink(g_entity_t *ent) {

	if (ent->locals.next_think == 0)
		return;

	if (ent->locals.next_think > g_level.time + 1)
		return;

	ent->locals.next_think = 0;

	if (!ent->locals.Think)
		gi.Error("%s has no Think function\n", etos(ent));

	ent->locals.Think(ent);
}

/**
 * @return True if the entitiy is in a valid position, false otherwise.
 */
static _Bool G_GoodPosition(const g_entity_t *ent) {

	const int32_t mask = ent->locals.clip_mask ? ent->locals.clip_mask : MASK_SOLID;

	const cm_trace_t tr = gi.Trace(ent->s.origin, ent->s.origin, ent->mins, ent->maxs, ent, mask);

	return tr.start_solid == false;
}

/**
 * @return True if the entitiy is in, or could be moved to, a valid position, false otherwise.
 */
static _Bool G_CorrectPosition(g_entity_t *ent) {

	const int32_t mask = ent->locals.clip_mask ?: MASK_SOLID;

	const cm_trace_t tr = gi.Trace(ent->s.origin, ent->s.origin, ent->mins, ent->maxs, ent, mask);
	if (tr.all_solid) {

		for (int32_t i = -1; i <= 1; i++) {
			for (int32_t j = -1; j <= 1; j++) {
				for (int32_t k = -1; k <= 1; k++) {
					vec3_t pos;
					VectorCopy(ent->s.origin, pos);

					pos[0] += i * PM_NUDGE_DIST;
					pos[1] += j * PM_NUDGE_DIST;
					pos[2] += k * PM_NUDGE_DIST;

					const cm_trace_t tr = gi.Trace(pos, pos, ent->mins, ent->maxs, ent, mask);
					if (!tr.all_solid) {
						VectorCopy(pos, ent->s.origin);
						return true;
					}
				}
			}
		}

		gi.Debug("still solid, reverting %s\n", etos(ent));
		return false;
	}

	return true;
}

#define MAX_SPEED 2400.0
#define STOP_EPSILON PM_STOP_EPSILON

/**
 * @brief
 */
static void G_ClampVelocity(g_entity_t *ent) {

	const vec_t speed = VectorLength(ent->locals.velocity);

	if (speed > MAX_SPEED) {
		VectorScale(ent->locals.velocity, MAX_SPEED / speed, ent->locals.velocity);
	}
	else if (speed < STOP_EPSILON) {
		VectorClear(ent->locals.velocity);
	}
}

/**
 * @brief Slide off of the impacted plane.
 */
static void G_ClipVelocity(const vec3_t in, const vec3_t normal, vec3_t out, vec_t bounce) {

	vec_t backoff = DotProduct(in, normal);

	if (backoff < 0.0)
		backoff *= bounce;
	else
		backoff /= bounce;

	for (int32_t i = 0; i < 3; i++) {

		const vec_t change = normal[i] * backoff;
		out[i] = in[i] - change;
	}
}

#define SPEED_STOP 150.0

/**
 * @see Pm_Friction
 */
static void G_Friction(g_entity_t *ent) {
	vec3_t vel;

	VectorCopy(ent->locals.velocity, vel);

	if (ent->locals.ground_entity) {
		vel[2] = 0.0;
	}

	const vec_t speed = VectorLength(vel);

	if (speed < 1.0) {
		VectorClear(ent->locals.velocity);
		return;
	}

	const vec_t control = MAX(SPEED_STOP, speed);

	vec_t friction = 0.0;

	if (ent->locals.ground_entity) {
		const cm_bsp_surface_t *surf = ent->locals.ground_surface;
		if (surf && (surf->flags & SURF_SLICK)) {
			friction = PM_FRICT_GROUND_SLICK;
		} else {
			friction = PM_FRICT_GROUND;
		}
	} else {
		friction = PM_FRICT_AIR;
	}

	friction += PM_FRICT_WATER * ent->locals.water_level;

	vec_t scale = MAX(0.0, speed - (friction * control * gi.frame_seconds)) / speed;

	VectorScale(ent->locals.velocity, scale, ent->locals.velocity);
	VectorScale(ent->locals.avelocity, scale, ent->locals.avelocity);
}

/**
 * @see Pm_Accelerate
 */
static void G_Accelerate(g_entity_t *ent, vec3_t dir, vec_t speed, vec_t accel) {

	const vec_t current_speed = DotProduct(ent->locals.velocity, dir);
	const vec_t add_speed = speed - current_speed;

	if (add_speed <= 0.0)
		return;

	vec_t accel_speed = accel * gi.frame_seconds * speed;

	if (accel_speed > add_speed)
		accel_speed = add_speed;

	gi.Print("%3.2f %3.2f %3.2f\n", current_speed, add_speed, accel_speed);

	VectorMA(ent->locals.velocity, accel_speed, dir, ent->locals.velocity);
}

/**
 * @see Pm_Gravity
 */
static void G_Gravity(g_entity_t *ent) {

	if (ent->locals.ground_entity == NULL) {
		vec_t gravity = g_level.gravity;

		if (ent->locals.water_level) {
			gravity *= PM_GRAVITY_WATER;
		}

		ent->locals.velocity[2] -= gravity * gi.frame_seconds;
	}
}

/**
 * @see Pm_Currents
 */
static void G_Currents(g_entity_t *ent) {
	vec3_t current;

	VectorClear(current);

	if (ent->locals.water_level) {

		if (ent->locals.water_type & CONTENTS_CURRENT_0)
			current[0] += 1.0;
		if (ent->locals.water_type & CONTENTS_CURRENT_90)
			current[1] += 1.0;
		if (ent->locals.water_type & CONTENTS_CURRENT_180)
			current[0] -= 1.0;
		if (ent->locals.water_type & CONTENTS_CURRENT_270)
			current[1] -= 1.0;
		if (ent->locals.water_type & CONTENTS_CURRENT_UP)
			current[2] += 1.0;
		if (ent->locals.water_type & CONTENTS_CURRENT_DOWN)
			current[2] -= 1.0;
	}

	if (ent->locals.ground_entity) {

		if (ent->locals.ground_contents & CONTENTS_CURRENT_0)
			current[0] += 1.0;
		if (ent->locals.ground_contents & CONTENTS_CURRENT_90)
			current[1] += 1.0;
		if (ent->locals.ground_contents & CONTENTS_CURRENT_180)
			current[0] -= 1.0;
		if (ent->locals.ground_contents & CONTENTS_CURRENT_270)
			current[1] -= 1.0;
		if (ent->locals.ground_contents & CONTENTS_CURRENT_UP)
			current[2] += 1.0;
		if (ent->locals.ground_contents & CONTENTS_CURRENT_DOWN)
			current[2] -= 1.0;
	}

	VectorScale(current, PM_SPEED_CURRENT, current);

	const vec_t speed = VectorNormalize(current);

	if (speed == 0.0)
		return;

	G_Accelerate(ent, current, speed, PM_ACCEL_GROUND);
}

/**
 * @brief Interact with `BOX_OCCUPY` objects after moving.
 */
void G_TouchOccupy(g_entity_t *ent) {
	g_entity_t *ents[MAX_ENTITIES];

	switch (ent->solid) {
		case SOLID_PROJECTILE:
		case SOLID_DEAD:
		case SOLID_BOX:
			break;
		default:
			return;
	}

	const size_t len = gi.BoxEntities(ent->abs_mins, ent->abs_maxs, ents, lengthof(ents), BOX_OCCUPY);
	for (size_t i = 0; i < len; i++) {

		g_entity_t *occupied = ents[i];

		if (occupied == ent)
			continue;

		if (occupied->locals.Touch) {
			gi.Debug("%s occupying %s\n", etos(ent), etos(occupied));
			occupied->locals.Touch(occupied, ent, NULL, NULL);
		}

		if (!ent->in_use)
			break;
	}
}

/**
 * @brief A moving object that doesn't obey physics
 */
static void G_Physics_NoClip(g_entity_t *ent) {

	VectorMA(ent->s.angles, gi.frame_seconds, ent->locals.avelocity, ent->s.angles);
	VectorMA(ent->s.origin, gi.frame_seconds, ent->locals.velocity, ent->s.origin);

	gi.LinkEntity(ent);
}

/**
 * @brief Maintain a record of all pushed entities for each move, in case that
 * move needs to be reverted.
 */
typedef struct {
	g_entity_t *ent;
	vec3_t origin;
	vec3_t angles;
	int16_t delta_yaw;
} g_push_t;

static g_push_t g_pushes[MAX_ENTITIES], *g_push_p;

/**
 * @brief
 */
static void G_Physics_Push_Impact(g_entity_t *ent) {

	if (g_push_p - g_pushes == MAX_ENTITIES)
		gi.Error("MAX_ENTITIES\n");

	g_push_p->ent = ent;

	VectorCopy(ent->s.origin, g_push_p->origin);
	VectorCopy(ent->s.angles, g_push_p->angles);

	if (ent->client) {
		g_push_p->delta_yaw = ent->client->ps.pm_state.delta_angles[YAW];
	} else {
		g_push_p->delta_yaw = 0;
	}

	g_push_p++;
}

/**
 * @brief
 */
static void G_Physics_Push_Revert(const g_push_t *p) {

	VectorCopy(p->origin, p->ent->s.origin);
	VectorCopy(p->angles, p->ent->s.angles);

	if (p->ent->client) {
		p->ent->client->ps.pm_state.delta_angles[YAW] = p->delta_yaw;
	}

	gi.LinkEntity(p->ent);
}

/**
 * @brief When items ride pushers, they rotate along with them. For clients,
 * this requires incrementing their delta angles.
 */
static void G_Physics_Push_Rotate(g_entity_t *self, g_entity_t *ent, vec_t yaw) {

	if (ent->locals.ground_entity == self) {
		if (ent->client) {
			g_client_t *cl = ent->client;

			yaw += UnpackAngle(cl->ps.pm_state.delta_angles[YAW]);

			cl->ps.pm_state.delta_angles[YAW] = PackAngle(yaw);
		} else {
			ent->s.angles[YAW] += yaw;
		}
	}
}

/**
 * @brief
 */
static g_entity_t *G_Physics_Push_Move(g_entity_t *self, vec3_t move, vec3_t amove) {
	vec3_t inverse_amove, forward, right, up;
	g_entity_t *ents[MAX_ENTITIES];

	G_Physics_Push_Impact(self);

	// move the pusher to it's intended position
	VectorAdd(self->s.origin, move, self->s.origin);
	VectorAdd(self->s.angles, amove, self->s.angles);

	gi.LinkEntity(self);

	// calculate the angle vectors for rotational movement
	VectorNegate(amove, inverse_amove);
	AngleVectors(inverse_amove, forward, right, up);

	// see if any solid entities are inside the final position
	const size_t len = gi.BoxEntities(self->abs_mins, self->abs_maxs, ents, lengthof(ents), BOX_ALL);
	for (size_t i = 0; i < len; i++) {

		g_entity_t *ent = ents[i];

		if (ent == self)
			continue;

		if (ent->solid == SOLID_BSP)
			continue;

		if (ent->locals.move_type < MOVE_TYPE_WALK)
			continue;

		// if the entity is in a good position and not riding us, we can skip them
		if (G_GoodPosition(ent) && ent->locals.ground_entity != self) {
			continue;
		}

		// if we are a pusher, or someone is riding us, try to move them
		if ((self->locals.move_type == MOVE_TYPE_PUSH) || (ent->locals.ground_entity == self)) {
			vec3_t translate, rotate, delta;

			G_Physics_Push_Impact(ent);

			// translate the pushed entity
			VectorAdd(ent->s.origin, move, ent->s.origin);

			// then rotate the movement to comply with the pusher's rotation
			VectorSubtract(ent->s.origin, self->s.origin, translate);

			rotate[0] = DotProduct(translate, forward);
			rotate[1] = -DotProduct(translate, right);
			rotate[2] = DotProduct(translate, up);

			VectorSubtract(rotate, translate, delta);

			VectorAdd(ent->s.origin, delta, ent->s.origin);
			
			// if the move has separated us, finish up by rotating the entity
			if (G_CorrectPosition(ent)) {
				G_Physics_Push_Rotate(self, ent, amove[YAW]);
				continue;
			}

			if (ent->locals.ground_entity == self) {
				
				// an entity riding us may have been pushed off of us by the world, so try
				// it's original position, which may now be valid
				
				G_Physics_Push_Revert(--g_push_p);

				// but in this case, don't rotate
				if (G_CorrectPosition(ent)) {
					continue;
				}
			}
		}

		// try to destroy the impeding entity by calling our Blocked function

		if (self->locals.Blocked) {
			self->locals.Blocked(self, ent);
			if (!ent->in_use || ent->locals.dead) {
				continue;
			}
		}

		gi.Debug("%s blocked by %s\n", etos(self), etos(ent));

		// if we've reached this point, we were G_MOVE_TYPE_STOP, or we were
		// blocked: revert any moves we may have made and return our obstacle

		while (g_push_p > g_pushes) {
			G_Physics_Push_Revert(--g_push_p);
		}

		return ent;
	}

	// the move was successful, so re-link all pushed entities
	for (g_push_t *p = g_push_p - 1; p >= g_pushes; p--) {
		if (p->ent->in_use) {

			gi.LinkEntity(p->ent);
			
			G_CheckGround(p->ent);
			
			G_CheckWater(p->ent);
			
			G_TouchOccupy(p->ent);
		}
	}

	return NULL;
}

/**
 * @brief For G_MOVE_TYPE_PUSH, push all box entities intersected while moving.
 * Generally speaking, only inline BSP models are pushers.
 */
static void G_Physics_Push(g_entity_t *ent) {
	g_entity_t *obstacle = NULL;

	// for teamed entities, the master must initiate all moves
	if (ent->locals.flags & FL_TEAM_SLAVE)
		return;

	// reset the pushed array
	g_push_p = g_pushes;

	// make sure all team slaves can move before committing any moves
	for (g_entity_t *part = ent; part; part = part->locals.team_chain) {
		if (!VectorCompare(part->locals.velocity, vec3_origin) ||
				!VectorCompare(part->locals.avelocity, vec3_origin)) { // object is moving

			vec3_t move, amove;

			VectorScale(part->locals.velocity, gi.frame_seconds, move);
			VectorScale(part->locals.avelocity, gi.frame_seconds, amove);

			if ((obstacle = G_Physics_Push_Move(part, move, amove)))
				break; // move was blocked
		}
	}

	if (obstacle) { // blocked, let's try again next frame
		for (g_entity_t *part = ent; part; part = part->locals.team_chain) {
			if (part->locals.next_think)
				part->locals.next_think += gi.frame_millis;
		}
	} else { // the move succeeded, so call all think functions
		for (g_entity_t *part = ent; part; part = part->locals.team_chain) {
			G_RunThink(part);
		}
	}
}

#define MAX_CLIP_PLANES 4

typedef struct {
	g_entity_t *entities[MAX_CLIP_PLANES];
	int32_t num_entities;
} g_touch_t;

static g_touch_t g_touch;

/**
 * @brief Runs the `Touch` functions of each object.
 */
static void G_TouchEntity(g_entity_t *ent, const cm_trace_t *trace) {
	
	// ensure that we only impact an entity once per frame

	for (int32_t i = 0; i < g_touch.num_entities; i++) {
		if (g_touch.entities[i] == trace->ent) {
			return;
		}
	}
	
	g_touch.entities[g_touch.num_entities++] = trace->ent;
	
	// run the interaction
	
	if (ent->locals.Touch) {
		gi.Debug("%s touching %s\n", etos(ent), etos(trace->ent));
		ent->locals.Touch(ent, trace->ent, &trace->plane, trace->surface);
	}

	if (ent->in_use && trace->ent->in_use) {
		
		if (trace->ent->locals.Touch) {
			gi.Debug("%s touching %s\n", etos(trace->ent), etos(ent));
			trace->ent->locals.Touch(trace->ent, ent, NULL, NULL);
		}
	}
}

/**
 * @see Pm_SlideMove
 */
static _Bool G_Physics_Fly_Move(g_entity_t *ent, const vec_t bounce) {
	vec3_t planes[MAX_CLIP_PLANES];
	vec3_t origin, angles;
	
	memset(&g_touch, 0, sizeof(g_touch));

	VectorCopy(ent->s.origin, origin);
	VectorCopy(ent->s.angles, angles);

	const int32_t mask = ent->locals.clip_mask ? ent->locals.clip_mask : MASK_SOLID;

	vec_t time_remaining = gi.frame_seconds;
	int32_t num_planes = 0;

	for (int32_t bump = 0; bump < MAX_CLIP_PLANES; bump++) {
		vec3_t pos;

		if (time_remaining <= 0.0)
			break;

		// project desired destination
		VectorMA(ent->s.origin, time_remaining, ent->locals.velocity, pos);

		// trace to it
		const cm_trace_t trace = gi.Trace(ent->s.origin, pos, ent->mins, ent->maxs, ent, mask);

		// if the entity is trapped in a solid, don't build up Z
		if (trace.all_solid) {
			ent->locals.velocity[2] = 0;
			return true;
		}

		const vec_t time = trace.fraction * time_remaining;

		VectorMA(ent->s.origin, time, ent->locals.velocity, ent->s.origin);
		VectorMA(ent->s.angles, time, ent->locals.avelocity, ent->s.angles);

		time_remaining -= time;

		if (trace.ent && trace.ent->solid > SOLID_TRIGGER) {

			G_TouchEntity(ent, &trace);

			if (!ent->in_use) {
				return true;
			}
			
			if (!trace.ent->in_use) {
				continue;
			}
			
			// if both entities remain, clip this entity to the trace entity
			
			VectorCopy(trace.plane.normal, planes[num_planes]);
			num_planes++;
			
			for (int32_t i = 0; i < num_planes; i++) {
				vec3_t vel;

				if (DotProduct(ent->locals.velocity, planes[i]) >= 0.0) {
					continue;
				}
				
				// slide along the plane
				G_ClipVelocity(ent->locals.velocity, planes[i], vel, bounce);
				
				// see if there is a second plane that the new move enters
				for (int32_t j = 0; j < num_planes; j++) {
					vec3_t cross;
					
					if (j == i) {
						continue;
					}
					
					if (DotProduct(vel, planes[j]) >= 0.0) {
						continue;
					}
					
					// try clipping the move to the plane
					G_ClipVelocity(vel, planes[j], vel, PM_CLIP_BOUNCE);
					
					// see if it goes back into the first clip plane
					if (DotProduct(vel, planes[i]) >= 0.0) {
						continue;
					}
					
					// slide the original velocity along the crease
					CrossProduct(planes[i], planes[j], cross);
					VectorNormalize(cross);
					
					const vec_t scale = DotProduct(cross, ent->locals.velocity);
					VectorScale(cross, scale, vel);
					
					// see if there is a third plane the the new move enters
					for (int32_t k = 0; k < num_planes; k++) {
						
						if (k == i || k == j) {
							continue;
						}
						
						if (DotProduct(vel, planes[k]) >= 0.0) {
							continue;
						}
						
						// stop dead at a triple plane interaction
						VectorClear(ent->locals.velocity);
						return true;
					}
				}
				
				// if we have fixed all interactions, try another move
				VectorCopy(vel, ent->locals.velocity);
				break;
			}
		}
	}
	
	if (!G_CorrectPosition(ent)) {
		gi.Debug("reverting %s\n", etos(ent));
		
		VectorCopy(origin, ent->s.origin);
		VectorCopy(angles, ent->s.angles);
		
		VectorClear(ent->locals.velocity);
		VectorClear(ent->locals.avelocity);
	}
	
	gi.LinkEntity(ent);
	
	return num_planes == 0;
}

/**
 * @brief Fly through the world, interacting with other solids.
 */
static void G_Physics_Fly(g_entity_t *ent) {

	G_Physics_Fly_Move(ent, 1.0);

	G_CheckGround(ent);
	
	G_CheckWater(ent);
	
	G_TouchOccupy(ent);
}

/**
 * @brief Bounce movement. When on ground, do nothing.
 */
static void G_Physics_Bounce(g_entity_t *ent) {
	
	if (ent->locals.ground_entity == NULL || VectorCompare(ent->locals.velocity, vec3_origin) == false) {

		G_Friction(ent);
		
		G_Gravity(ent);
		
		G_Currents(ent);
		
		G_Physics_Fly_Move(ent, 1.33);
	}

	G_CheckGround(ent);
	
	G_CheckWater(ent);
	
	G_TouchOccupy(ent);
}

/**
 * @brief Dispatches thinking and physics routines for the specified entity.
 */
void G_RunEntity(g_entity_t *ent) {

	G_ClampVelocity(ent);

	G_RunThink(ent);

	switch (ent->locals.move_type) {
		case MOVE_TYPE_NONE:
			break;
		case MOVE_TYPE_NO_CLIP:
			G_Physics_NoClip(ent);
			break;
		case MOVE_TYPE_PUSH:
		case MOVE_TYPE_STOP:
			G_Physics_Push(ent);
			break;
		case MOVE_TYPE_FLY:
			G_Physics_Fly(ent);
			break;
		case MOVE_TYPE_BOUNCE:
			G_Physics_Bounce(ent);
			break;
		default:
			gi.Error("Bad move type %i\n", ent->locals.move_type);
			break;
	}

	// move all team members to the new origin
	g_entity_t *e = ent->locals.team_chain;
	while (e) {
		VectorCopy(ent->s.origin, e->s.origin);

		gi.LinkEntity(e);

		e = e->locals.team_chain;
	}

	// update BSP sub-model animations based on move state
	if (ent->solid == SOLID_BSP) {
		ent->s.animation1 = ent->locals.move_info.state;
	}
}
