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

#include "ai_local.h"
#include "game/default/bg_pmove.h"
#include "game/default/g_ai.h"

/**
 * @brief AI imports.
 */
ai_import_t aim;

/**
 * @brief AI exports.
 */
ai_export_t aix;

ai_level_t ai_level;

ai_entity_data_t ai_entity_data;
ai_client_data_t ai_client_data;

cvar_t *sv_max_clients;
cvar_t *ai_no_target;
cvar_t *ai_node_dev;

/**
 * @brief Ptr to AI locals that are hooked to bot entities.
 */
static ai_locals_t *ai_locals;

/**
 * @brief Get the locals for the specified bot entity.
 */
ai_locals_t *Ai_GetLocals(const g_entity_t *ent) {
	return ai_locals + (ent->s.number - 1);
}

/**
 * @brief
 */
static _Bool Ai_CanSee(const g_entity_t *self, const g_entity_t *other) {

	// see if we're even facing the object
	ai_locals_t *ai = Ai_GetLocals(self);

	const vec3_t dir = Vec3_Normalize(Vec3_Subtract(other->s.origin, ai->eye_origin));

	float dot = Vec3_Dot(ai->aim_forward, dir);

	if (dot < 0.5) {
		return false;
	}

	cm_trace_t tr = aim.gi->Trace(ai->eye_origin, other->s.origin, Vec3_Zero(), Vec3_Zero(), self, CONTENTS_MASK_CLIP_PROJECTILE);

	if (!Vec3_BoxIntersect(tr.end, tr.end, other->abs_mins, other->abs_maxs)) {
		return false; // something was in the way of our trace
	}

	// got it!
	return true;
}

/**
 * @brief
 */
static _Bool Ai_CanTarget(const g_entity_t *self, const g_entity_t *other) {

	if (other == self || !other->in_use || other->solid == SOLID_DEAD || other->solid == SOLID_NOT ||
	        (other->sv_flags & SVF_NO_CLIENT) || (other->client && aim.OnSameTeam(self, other))) {
		return false;
	}

	return Ai_CanSee(self, other);
}

/**
 * @brief Executes a console command as if the bot typed it.
 */
static void Ai_Command(g_entity_t *self, const char *command) {

	aim.gi->TokenizeString(command);
	aim.ge->ClientCommand(self);
}

/**
 * @brief The max distance we'll try to hunt an item at.
 */
#define AI_MAX_ITEM_DISTANCE 512

/**
 * @brief
 */
typedef struct {
	const g_entity_t *entity;
	const g_item_t *item;
	float weight;
} ai_item_pick_t;

/**
 * @brief
 */
static int32_t Ai_ItemPick_Compare(const void *a, const void *b) {

	const ai_item_pick_t *w0 = (const ai_item_pick_t *) a;
	const ai_item_pick_t *w1 = (const ai_item_pick_t *) b;

	return SignOf(w1->weight - w0->weight);
}

#define AI_ITEM_UNREACHABLE -1.0

/**
 * @brief
 */
static float Ai_ItemReachable(const g_entity_t *self, const g_entity_t *other) {

	vec3_t line = Vec3_Subtract(self->s.origin, other->s.origin);

	if (fabs(line.z) >= PM_STEP_HEIGHT) {
		return AI_ITEM_UNREACHABLE;
	}

	const float distance = Vec3_Length(line);

	if (distance >= AI_MAX_ITEM_DISTANCE) {
		return AI_ITEM_UNREACHABLE;
	}

	// if the distance is over a chasm, let's see if we can even reach it
	if (distance >= 32.0) {

		vec3_t fall_start;
		fall_start = Vec3_Add(self->s.origin, other->s.origin);
		fall_start = Vec3_Scale(fall_start, 0.5);

		vec3_t fall_end;
		fall_end = fall_start;
		fall_end.z -= PM_STEP_HEIGHT * 2.0;

		cm_trace_t tr = aim.gi->Trace(fall_start, fall_end, Vec3_Zero(), Vec3_Zero(), NULL, CONTENTS_SOLID);

		if (tr.start_solid || tr.all_solid || tr.fraction == 1.0) {
			return AI_ITEM_UNREACHABLE;
		}
	}

	return distance;
}

/**
 * @brief
 */
static void Ai_ResetWander(const g_entity_t *self, const vec3_t where_to) {
	ai_locals_t *ai = Ai_GetLocals(self);
	vec3_t dir;

	dir = Vec3_Subtract(where_to, self->s.origin);
	dir = Vec3_Normalize(dir);
	dir = Vec3_Euler(dir);

	ai->wander_angle = dir.y;
}

/**
 * @brief Seek for items if we're not doing anything better.
 */
static uint32_t Ai_FuncGoal_FindItems(g_entity_t *self, pm_cmd_t *cmd) {

	if (self->solid == SOLID_DEAD) {
		return QUETOO_TICK_MILLIS;
	}

	ai_locals_t *ai = Ai_GetLocals(self);

	// if we got stuck, don't hunt for items for a little bit
	if (ai->reacquire_time > ai_level.time) {
		return ai->reacquire_time - ai_level.time; 
	}

	// see if we're already hunting
	if (ai->move_target.type == AI_GOAL_ENEMY) {
		return QUETOO_TICK_MILLIS * 5;
	}

	if (ai->move_target.type == AI_GOAL_ITEM) {

		// check to see if the item has gone out of our line of sight
		if (!Ai_GoalHasEntity(&ai->move_target, ai->move_target.ent) || // item picked up and changed into something else
				!Ai_CanTarget(self, ai->move_target.ent) ||
				!Ai_CanPickupItem(self, ai->move_target.ent) ||
		        Ai_ItemReachable(self, ai->move_target.ent) == AI_ITEM_UNREACHABLE) {

			Ai_ResetWander(self, ai->move_target.ent->s.origin);

			if (ai->aim_target.ent == ai->move_target.ent) {
				Ai_ClearGoal(&ai->aim_target);
			}

			Ai_ClearGoal(&ai->move_target);
		} else if (ai->aim_target.type <= AI_GOAL_GHOST || Randomf() < 0.3f) { // aim towards item sometimes so it doesn't look like we're a feckin' cheater
			Ai_CopyGoal(&ai->move_target, &ai->aim_target);
		}
	}

	// still have an item
	if (ai->move_target.type == AI_GOAL_ITEM) {
		return QUETOO_TICK_MILLIS * 3;
	}

	// if we're aiming for an active target, don't look for new items
	if (ai->aim_target.type == AI_GOAL_ENEMY) {
		return QUETOO_TICK_MILLIS * 3;
	}

	// we lost our enemy, start looking for a new one
	GArray *items_visible = g_array_new(false, false, sizeof(ai_item_pick_t));

	for (int32_t i = sv_max_clients->integer + 1; i < MAX_ENTITIES; i++) {

		g_entity_t *ent = ENTITY_FOR_NUM(i);

		if (!ent->in_use) {
			continue;
		}

		if (ent->s.solid != SOLID_TRIGGER) {
			continue;
		}

		const g_item_t *item = ENTITY_DATA(ent, item);

		if (!item) {
			continue;
		}

		// most likely an item!
		float distance;

		if (!Ai_CanTarget(self, ent) ||
				!Ai_CanPickupItem(self, ent) ||
		        (distance = Ai_ItemReachable(self, ent)) == AI_ITEM_UNREACHABLE) {
			continue;
		}

		g_array_append_vals(items_visible, &(const ai_item_pick_t) {
			.entity = ent,
			 .item = item,
			  .weight = (AI_MAX_ITEM_DISTANCE - distance) * ITEM_DATA(item, priority)
		}, 1);
	}

	// found one, set it up
	if (items_visible->len) {

		if (items_visible->len > 1) {
			g_array_sort(items_visible, Ai_ItemPick_Compare);
		}

		Ai_SetEntityGoal(&ai->move_target, AI_GOAL_ITEM, 1.0, g_array_index(items_visible, ai_item_pick_t, 0).entity);

		g_array_free(items_visible, true);

		// re-run with the new target
		return Ai_FuncGoal_FindItems(self, cmd);
	}

	g_array_free(items_visible, true);

	return QUETOO_TICK_MILLIS * 5;
}

/**
 * @brief Range constants.
 */
typedef enum {
	RANGE_DONT_CARE = 0,
	RANGE_MELEE = 32,
	RANGE_SHORT = 128,
	RANGE_MED = 512,
	RANGE_LONG = 1024
} ai_range_t;

/**
 * @brief
 */
static ai_range_t Ai_GetRange(const float distance) {
	if (distance < RANGE_MELEE) {
		return RANGE_MELEE;
	} else if (distance < RANGE_SHORT) {
		return RANGE_SHORT;
	} else if (distance < RANGE_MED) {
		return RANGE_MED;
	}

	return RANGE_LONG;
}

/**
 * @brief Picks a weapon for the AI based on its target
 */
static void Ai_PickBestWeapon(g_entity_t *self) {
	ai_locals_t *ai = Ai_GetLocals(self);

	ai->weapon_check_time = ai_level.time + 250; // don't try again for a bit

	ai_range_t targ_range;

	if (ai->aim_target.type == AI_GOAL_ENEMY) {
		targ_range = Ai_GetRange(Vec3_Distance(self->s.origin, ai->aim_target.ent->s.origin));
	} else {
		targ_range = RANGE_DONT_CARE;
	}

	ai_item_pick_t weapons[ai_num_weapons];
	uint16_t num_weapons = 0;

	const int16_t *inventory = &CLIENT_DATA(self->client, inventory);

	for (uint16_t i = 0; i < ai_num_items; i++) {
		const g_item_t *item = ai_items[i];

		if (ITEM_DATA(item, type) != ITEM_WEAPON) { // not weapon
			continue;
		}

		if (!inventory[ITEM_DATA(item, index)]) { // not in stock
			continue;
		}

		const g_item_t *ammo = aim.G_FindItem(ITEM_DATA(item, ammo));
		if (ammo) {
			const int32_t ammo_have = inventory[ITEM_DATA(ammo, index)];
			const int32_t ammo_need = ITEM_DATA(item, quantity);
			if (ammo_have < ammo_need) {
				continue;
			}
		}

		// calculate weight, start with base weapon priority
		float weight = ITEM_DATA(item, priority);

		switch (targ_range) { // range bonus
			case RANGE_DONT_CARE:
				break;
			case RANGE_MELEE:
			case RANGE_SHORT:
				if (ITEM_DATA(item, flags) & WF_SHORT_RANGE) {
					weight *= 2.5;
				} else {
					weight /= 2.5;
				}
				break;
			case RANGE_MED:
				if (ITEM_DATA(item, flags) & WF_MED_RANGE) {
					weight *= 2.5;
				} else {
					weight /= 2.5;
				}
				break;
			case RANGE_LONG:
				if (ITEM_DATA(item, flags) & WF_LONG_RANGE) {
					weight *= 2.5;
				} else {
					weight /= 2.5;
				}
				break;
		}

		if (ai->aim_target.type == AI_GOAL_ENEMY) {
			if ((ENTITY_DATA(ai->aim_target.ent, health) < 25) &&
					(ITEM_DATA(item, flags) & WF_EXPLOSIVE)) { // bonus for explosive at low enemy health
				weight *= 1.5;
			}
		}

		// additional penalty for long range + projectile unless explicitly long range
		if ((ITEM_DATA(item, flags) & WF_PROJECTILE) &&
		        !(ITEM_DATA(item, flags) & WF_LONG_RANGE)) {
			weight /= 2.0;
		}

		// penalty for explosive weapons at short range
		if ((ITEM_DATA(item, flags) & WF_EXPLOSIVE) &&
		        (targ_range != RANGE_DONT_CARE && targ_range <= RANGE_SHORT)) {
			weight /= 2.0;
		}

		// penalty for explosive weapons at low self health
		if ((ENTITY_DATA(self, health) < 25) &&
		        (ITEM_DATA(item, flags) & WF_EXPLOSIVE)) {
			weight /= 2.0;
		}

		weapons[num_weapons++] = (ai_item_pick_t) {
			.item = item,
			.weight = weight
		};
		aim.gi->Debug("weapon choice: %s -> %f\n", ITEM_DATA(item, class_name), weight);
	}

	if (num_weapons <= 1) { // if we only have 1 here, we're already using it
		return;
	}

	qsort(weapons, num_weapons, sizeof(ai_item_pick_t), Ai_ItemPick_Compare);

	const ai_item_pick_t *best_weapon = &weapons[0];

	if (CLIENT_DATA(self->client, weapon) == best_weapon->item) {
		return;
	}

	Ai_Command(self, va("use %s", ITEM_DATA(best_weapon->item, name)));
	ai->weapon_check_time = ai_level.time + 300; // don't try again for a bit
}

/**
 * @brief Calculate a priority for the specified target.
 */
static float Ai_EnemyPriority(const g_entity_t *self, const g_entity_t *target, const _Bool visible) {

	// TODO: all of this function. Enemies with more powerful weapons need a higher weight.
	// Enemies with lower health need a higher weight. Enemies carrying flags need an even higher weight.

	return 10.0;
}

/**
 * @brief Funcgoal that controls the AI's lust for blood
 */
static uint32_t Ai_FuncGoal_Hunt(g_entity_t *self, pm_cmd_t *cmd) {

	if (self->solid == SOLID_DEAD) {
		return QUETOO_TICK_MILLIS;
	}

	ai_locals_t *ai = Ai_GetLocals(self);

	if (ai_no_target->integer) {

		if (ai->aim_target.type == AI_GOAL_ENEMY) {
			Ai_ClearGoal(&ai->aim_target);
		}

		if (ai->move_target.type == AI_GOAL_ENEMY) {
			Ai_ClearGoal(&ai->move_target);
		}

		return QUETOO_TICK_MILLIS;
	}

	// see if we're already hunting
	if (ai->aim_target.type == AI_GOAL_ENEMY) {

		// check to see if the enemy has gone out of our line of sight
		if (!Ai_CanTarget(self, ai->aim_target.ent)) {

			// enemy went out of our LOS, aim at where they were for a while
			if (ai->aim_target.ent->solid != SOLID_DEAD && !(ai->aim_target.ent->sv_flags & SVF_NO_CLIENT)) {
				Ai_SetEntityGoal(&ai->aim_target, AI_GOAL_GHOST, Ai_EnemyPriority(self, ai->aim_target.ent, false), ai->aim_target.ent);
				ai->ghost_position = ai->aim_target.ent->s.origin;
				Ai_ResetWander(self, ai->ghost_position);
			} else {
				Ai_ClearGoal(&ai->aim_target);
			}

			Ai_ClearGoal(&ai->move_target);
		}

		// TODO: we should change targets here based on priority.
	}

	// if we're wandering/not looking for something, move towards the target
	if (ai->move_target.type <= AI_GOAL_NAV) {

		// TODO: if we have proper navigation, then we can remove this if statement.
		// until then, we only try to move towards actual enemies and not ghosts.
		if (ai->aim_target.type == AI_GOAL_ENEMY) {
			Ai_CopyGoal(&ai->aim_target, &ai->move_target);
		}
	}

	// still have an enemy
	if (ai->aim_target.type == AI_GOAL_ENEMY) {

		return QUETOO_TICK_MILLIS * 3;
	}

	// we lost our enemy, start looking for a new one
	g_entity_t *player = NULL;

	for (int32_t i = 1; i <= sv_max_clients->integer; i++) {

		g_entity_t *ent = ENTITY_FOR_NUM(i);

		if (Ai_CanTarget(self, ent)) {

			player = ent;
			break;
		}
	}

	// found one, set it up
	if (player) {

		Ai_SetEntityGoal(&ai->aim_target, AI_GOAL_ENEMY, Ai_EnemyPriority(self, player, true), player);

		Ai_PickBestWeapon(self);

		ai->combat_type = RandomRangei(AI_COMBAT_CLOSE, AI_COMBAT_TOTAL);
		ai->lock_on_time = ai_level.time + RandomRangeu(250, 1000);

		if (ai->combat_type == AI_COMBAT_FLANK) {
			ai->wander_angle = Randomb() ? -90 : 90;
		}

		// re-run hunt with the new target
		return Ai_FuncGoal_Hunt(self, cmd);
	}

	return QUETOO_TICK_MILLIS * 5;
}

/**
 * @brief Funcgoal that controls the AI's weaponry.
 */
static uint32_t Ai_FuncGoal_Weaponry(g_entity_t *self, pm_cmd_t *cmd) {

	// if we're dead, just keep clicking so we respawn.
	if (self->solid == SOLID_DEAD) {

		if ((ai_level.frame_num % 2) == 0) {
			cmd->buttons = BUTTON_ATTACK;
		}
		return QUETOO_TICK_MILLIS;
	}

	ai_locals_t *ai = Ai_GetLocals(self);

	if (ai->weapon_check_time < ai_level.time) { // check for a new weapon every once in a while
		Ai_PickBestWeapon(self);
	}

	// we're alive - if we're aiming at an enemy, start-a-firin
	if (ai->aim_target.type == AI_GOAL_ENEMY) {
		if (ai->lock_on_time < ai_level.time) {

			const uint32_t hand_nade_time = CLIENT_DATA(self->client, grenade_hold_time);

			if (hand_nade_time) {
				if ((ai_level.time - hand_nade_time) < RandomRangei(1500, 2500)) {
					cmd->buttons |= BUTTON_ATTACK;
				}
			}
			else {
				cmd->buttons |= BUTTON_ATTACK;
			}
		}
	}

	return QUETOO_TICK_MILLIS;
}

/**
 * @brief Funcgoal that controls the AI's crouch/jumping while hunting.
 */
static uint32_t Ai_FuncGoal_Acrobatics(g_entity_t *self, pm_cmd_t *cmd) {

	if (self->solid == SOLID_DEAD) {
		return QUETOO_TICK_MILLIS;
	}

	ai_locals_t *ai = Ai_GetLocals(self);

	if (ai->aim_target.type != AI_GOAL_ENEMY) {
		return QUETOO_TICK_MILLIS * 5;
	}

	// do some acrobatics
	if (ENTITY_DATA(self, ground_entity)) {

		if (self->client->ps.pm_state.flags & PMF_DUCKED) {

			if ((RandomRangeu(0, 32)) == 0) { // uncrouch eventually
				cmd->up = 0;
			} else {
				cmd->up = -PM_SPEED_JUMP;
			}
		} else {

			if ((RandomRangeu(0, 32)) == 0) { // randomly crouch
				cmd->up = -PM_SPEED_JUMP;
			} else if ((RandomRangeu(0, 86)) == 0) { // randomly pop, to confuse our enemies
				cmd->up = PM_SPEED_JUMP;
			}
		}
	} else {

		cmd->up = 0;
	}

	return QUETOO_TICK_MILLIS;
}

/**
 * @brief Wander aimlessly, hoping to find something to love.
 */
static void Ai_Wander(g_entity_t *self, pm_cmd_t *cmd) {
	ai_locals_t *ai = Ai_GetLocals(self);

	vec3_t forward;
	Vec3_Vectors(Vec3(0.f, ai->wander_angle, 0.f), &forward, NULL, NULL);

	vec3_t end = Vec3_Add(self->s.origin, Vec3_Scale(forward, (self->maxs.x - self->mins.x) * 2.0));

	cm_trace_t tr = aim.gi->Trace(self->s.origin, end, Vec3_Zero(), Vec3_Zero(), self, CONTENTS_MASK_CLIP_PLAYER);

	if (tr.fraction < 1.0) { // hit a wall
		float angle = 45 + Randomf() * 45;
			
		if (ai->combat_type == AI_COMBAT_FLANK && Randomb()) {
			ai->combat_type = Randomb() ? AI_COMBAT_CLOSE : AI_COMBAT_WANDER;
		}

		if (ai->combat_type == AI_COMBAT_FLANK) {
			ai->wander_angle = -ai->wander_angle;
		} else {
			ai->wander_angle += Randomb() ? -angle : angle;
		}
	}
}

static g_entity_t *ai_current_entity;

/**
 * @brief Ignore ourselves, clipping to the correct mask based on our status.
 */
static cm_trace_t Ai_ClientMove_Trace(const vec3_t start, const vec3_t end, const vec3_t mins,
                                     const vec3_t maxs) {

	const g_entity_t *self = ai_current_entity;

	if (self->solid == SOLID_DEAD) {
		return aim.gi->Trace(start, end, mins, maxs, self, CONTENTS_MASK_CLIP_CORPSE);
	} else {
		return aim.gi->Trace(start, end, mins, maxs, self, CONTENTS_MASK_CLIP_PLAYER);
	}
}

/**
 * @brief Move towards our current target
 */
static void Ai_MoveToTarget(g_entity_t *self, pm_cmd_t *cmd) {

	ai_locals_t *ai = Ai_GetLocals(self);

	_Bool target_enemy = false;

	vec3_t dir, angles, dest = Vec3_Zero();
	_Bool move_wander = false;

	switch (ai->move_target.type) {
		default:
			move_wander = true;
			break;
		case AI_GOAL_ITEM:
			dest = ai->move_target.ent->s.origin;
			break;
		case AI_GOAL_ENEMY:
			target_enemy = true;

			switch (ai->combat_type) {
			case AI_COMBAT_CLOSE:
			default:
				dest = ai->move_target.ent->s.origin;
				break;
			case AI_COMBAT_FLANK:
			case AI_COMBAT_WANDER:
				move_wander = true;
				break;
			}

			break;
	}
	
	// TODO: node navigation.
	if (move_wander) {
		Ai_Wander(self, cmd);

		angles = Vec3(0.0, ai->wander_angle, 0.0);
		Vec3_Vectors(angles, &dir, NULL, NULL);
		dest = Vec3_Add(self->s.origin, Vec3_Scale(dir, 1.0));
	}

	dir = Vec3_Subtract(dest, self->s.origin);
	float len = Vec3_Length(dir);
	dir = Vec3_Normalize(dir);

    angles = Vec3_Euler(dir);

    const float delta_yaw = (CLIENT_DATA(self->client, angles)).y - angles.y;
    Vec3_Vectors(Vec3(0.0, delta_yaw, 0.0), &dir, NULL, NULL);

    dir = Vec3_Scale(dir, PM_SPEED_RUN);

    if (target_enemy && len < 200.0) {
		// switch to flank/wander, this helps us recover from being up close to enemies
		if (ai->combat_type == AI_COMBAT_CLOSE && Randomb()) {
			ai->combat_type = Randomf() > 0.5f ? AI_COMBAT_FLANK : AI_COMBAT_WANDER;
		}
	}

	cmd->forward = dir.x;
	cmd->right = dir.y;

    if (ENTITY_DATA(self, water_level) >= WATER_WAIST) {
        cmd->up = PM_SPEED_JUMP;
    }

    ai_current_entity = self;

    // predict ahead
    pm_move_t pm;

    memset(&pm, 0, sizeof(pm));
    pm.s = self->client->ps.pm_state;

    pm.s.origin = self->s.origin;

    /*if (self->client->locals.hook_pull) {

        if (self->client->locals.persistent.hook_style == HOOK_SWING) {
            pm.s.type = PM_HOOK_SWING;
        } else {
            pm.s.type = PM_HOOK_PULL;
        }
    } else {*/
        pm.s.velocity = ENTITY_DATA(self, velocity);
    /*}*/

    pm.s.type = PM_NORMAL;

    pm.cmd = *cmd;
    pm.ground_entity = ENTITY_DATA(self, ground_entity);
    //pm.hook_pull_speed = g_hook_pull_speed->value;

    pm.PointContents = aim.gi->PointContents;
    pm.Trace = Ai_ClientMove_Trace;

    pm.Debug = aim.gi->PmDebug_;

    // perform a move; predict a few frames ahead
    for (int32_t i = 0; i < 8; ++i) {
        Pm_Move(&pm);
    }

	// we weren't trying to jump and predicted ground is gone
    if (pm.cmd.up <= 0 && ENTITY_DATA(self, ground_entity) && !pm.ground_entity) {
        if (move_wander) {
            float angle = 45 + Randomf() * 45;

			ai->combat_type = Randomf() > 0.5f ? AI_COMBAT_CLOSE : AI_COMBAT_WANDER;
            ai->wander_angle += (Randomf() < 0.5) ? -angle : angle;
        } else {
            cmd->forward = cmd->right = 0; // stop for now
        }
    }
	
	// check for getting stuck
	vec3_t move_dir;
	move_dir = Vec3_Subtract(ai->last_origin, self->s.origin);
	move_dir.z = 0.0;
	float move_len = Vec3_Length(move_dir);

	if (move_len < (PM_SPEED_RUN * QUETOO_TICK_SECONDS) / 8.0) {
		ai->no_movement_frames++;
		ai->reacquire_time = ai_level.time + 1000;

		if (ai->no_movement_frames >= QUETOO_TICK_RATE) { // just turn around
			float angle = 45 + Randomf() * 45;
			
			if (ai->combat_type == AI_COMBAT_FLANK) {
				ai->combat_type = Randomb() ? AI_COMBAT_CLOSE : AI_COMBAT_WANDER;
			}

			ai->wander_angle += Randomb() ? -angle : angle;

			ai->no_movement_frames = 0;
		} else if (ai->no_movement_frames >= QUETOO_TICK_RATE / 2.0) { // try a jump first
			cmd->up = PM_SPEED_JUMP;
		}
	} else {
		ai->no_movement_frames = 0;
	}
}

static float AngleMod(const float a) {
	return (360.0 / 65536) * ((int32_t) (a * (65536 / 360.0)) & 65535);
}

static float Ai_CalculateAngle(g_entity_t *self, const float speed, float current, float ideal) {
	current = AngleMod(current);
	ideal = AngleMod(ideal);

	if (current == ideal) {
		return current;
	}

	float move = ideal - current;

	if (ideal > current) {
		if (move >= 180.0) {
			move = move - 360.0;
		}
	} else {
		if (move <= -180.0) {
			move = move + 360.0;
		}
	}

	if (move > 0) {
		if (move > speed) {
			move = speed;
		}
	} else {
		if (move < -speed) {
			move = -speed;
		}
	}

	return AngleMod(current + move);
}

/**
 * @brief Turn/look towards our current target
 */
static void Ai_TurnToTarget(g_entity_t *self, pm_cmd_t *cmd) {
	ai_locals_t *ai = Ai_GetLocals(self);
	ai_goal_t *aim_target = &ai->aim_target;

	vec3_t ideal_angles;

	// TODO: node navigation
	if (aim_target->type <= AI_GOAL_NAV || aim_target->type == AI_GOAL_ITEM) {
		ideal_angles = Vec3(0.0, ai->wander_angle, 0.0);
	} else {
		vec3_t aim_direction;

		if (aim_target->type == AI_GOAL_GHOST) {
			aim_direction = Vec3_Subtract(ai->ghost_position, self->s.origin);
		} else {
			
			aim_direction = Vec3_Subtract(aim_target->ent->s.origin, self->s.origin);

			const g_item_t *const weapon = CLIENT_DATA(self->client, weapon);

			if (ITEM_DATA(weapon, flags) & WF_PROJECTILE) {
				const float dist = Vec3_Length(aim_direction);
				// FIXME: *real* projectile speed generally factors into this...
				const float speed = RandomRangef(900, 1200);
				const float time = dist / speed;
				const vec3_t target_velocity = ENTITY_DATA(aim_target->ent, velocity);
				const vec3_t target_pos = Vec3_Add(aim_target->ent->s.origin, Vec3_Scale(target_velocity, time));
				aim_direction = Vec3_Subtract(target_pos, self->s.origin);
			} else {
				aim_direction = Vec3_Subtract(aim_target->ent->s.origin, self->s.origin);
			}
		}

		aim_direction = Vec3_Normalize(aim_direction);
		ideal_angles = Vec3_Euler(aim_direction);

		// fuzzy angle
		ideal_angles.x += sinf(ai_level.time / 128.0) * 4.3;
		ideal_angles.y += cosf(ai_level.time / 164.0) * 4.0;
	}

	const vec3_t view_angles = CLIENT_DATA(self->client, angles);

	for (int32_t i = 0; i < 2; ++i) {
		ideal_angles.xyz[i] = Ai_CalculateAngle(self, 6.5, view_angles.xyz[i], ideal_angles.xyz[i]);
	}

	cmd->angles = Vec3_Subtract(ideal_angles, self->client->ps.pm_state.delta_angles);
}

/**
 * @brief Called every frame for every AI.
 */
static void Ai_Think(g_entity_t *self, pm_cmd_t *cmd) {

	ai_locals_t *ai = Ai_GetLocals(self);

	if (self->solid == SOLID_DEAD) {
		Ai_ClearGoal(&ai->aim_target);
		Ai_ClearGoal(&ai->move_target);
	} else {
		Vec3_Vectors(CLIENT_DATA(self->client, angles), &ai->aim_forward, NULL, NULL);
		ai->eye_origin = Vec3_Add(self->s.origin, self->client->ps.pm_state.view_offset);
	}

	// run functional goals
	for (int32_t i = 0; i < MAX_AI_FUNCGOALS; i++) {
		ai_funcgoal_t *funcgoal = &ai->funcgoals[i];

		if (!funcgoal->think) {
			continue;
		}

		if (funcgoal->nextthink > ai_level.time) {
			continue;
		}

		uint32_t time = funcgoal->think(self, cmd);

		if (time == AI_GOAL_COMPLETE) {
			Ai_RemoveFuncGoal(self, funcgoal->think);
		} else {
			funcgoal->nextthink = ai_level.time + time;
		}
	}

	if (self->solid != SOLID_DEAD) {
		// FIXME: should these be funcgoals?
		// they'd have to be appended to the end of the list always
		// and we can't really enforce that.
		Ai_TurnToTarget(self, cmd);
		Ai_MoveToTarget(self, cmd);
	}

	ai->last_origin = self->s.origin;
}

/**
 * @brief Called every time an AI spawns
 */
static void Ai_Spawn(g_entity_t *self) {
	ai_locals_t *ai = Ai_GetLocals(self);
	memset(ai, 0, sizeof(*ai));

	if (self->solid == SOLID_NOT) { // intermission, spectator, etc
		return;
	}

	Ai_AddFuncGoal(self, Ai_FuncGoal_Hunt, 0);
	Ai_AddFuncGoal(self, Ai_FuncGoal_Weaponry, 0);
	Ai_AddFuncGoal(self, Ai_FuncGoal_Acrobatics, 0);
	Ai_AddFuncGoal(self, Ai_FuncGoal_FindItems, 0);
}

/**
 * @brief Called when an AI is first spawned and is ready to go.
 */
static void Ai_Begin(g_entity_t *self) {
}

static void Ai_State(uint32_t frame_num) {
	ai_level.frame_num = frame_num;
	ai_level.time = ai_level.frame_num * QUETOO_TICK_MILLIS;
}

/**
 * @brief Advance the bot simulation one frame.
 */
static void Ai_Frame(void) {

}

/**
 * @brief Sets up level state for bots.
 */
static void Ai_GameStarted(void) {

	ai_level.gameplay = atoi(aim.gi->GetConfigString(CS_GAMEPLAY));
	ai_level.match = atoi(aim.gi->GetConfigString(CS_MATCH));
	ai_level.ctf = atoi(aim.gi->GetConfigString(CS_CTF));
	ai_level.teams = atoi(aim.gi->GetConfigString(CS_TEAMS));
}

/**
 * @brief Sets up data offsets to local game data
 */
static void Ai_SetDataPointers(ai_entity_data_t *entity, ai_client_data_t *client, ai_item_data_t *item) {

	ai_entity_data = *entity;
	ai_client_data = *client;
	ai_item_data = *item;
}

/**
 * @brief Initializes the AI subsystem.
 */
static void Ai_Init(void) {

	aim.gi->Print("Ai module initialization...\n");
	aim.gi->Mkdir("ai");
	aim.gi->Mkdir("ai_nav");

	const char *s = va("%s %s %s", VERSION, BUILD_HOST, REVISION);
	cvar_t *ai_version = aim.gi->AddCvar("ai_version", s, CVAR_NO_SET, NULL);

	aim.gi->Print("  Version:    ^2%s^7\n", ai_version->string);

	sv_max_clients = aim.gi->GetCvar("sv_max_clients");

	ai_no_target = aim.gi->AddCvar("ai_no_target", "0", CVAR_DEVELOPER, "Disables bots targeting enemies");

	ai_node_dev = aim.gi->AddCvar("ai_node_dev", "0", CVAR_DEVELOPER | CVAR_LATCH, "Toggles node development mode");

	ai_locals = (ai_locals_t *) aim.gi->Malloc(sizeof(ai_locals_t) * sv_max_clients->integer, MEM_TAG_AI);

	Ai_InitItems();
	Ai_InitSkins();

	aim.gi->Print("Ai module initialized\n");
}

/**
 * @brief Shuts down the AI subsystem
 */
static void Ai_Shutdown(void) {

	aim.gi->Print("  ^5Ai module shutdown...\n");

	Ai_ShutdownItems();
	Ai_ShutdownSkins();

	aim.gi->FreeTag(MEM_TAG_AI);
}

/**
 * @brief Load the AI subsystem.
 */
ai_export_t *Ai_LoadAi(ai_import_t *import) {

	aim = *import;

	aix.api_version = AI_API_VERSION;

	aix.Init = Ai_Init;
	aix.Shutdown = Ai_Shutdown;

	aix.State = Ai_State;
	aix.Frame = Ai_Frame;

	aix.GetUserInfo = Ai_GetUserInfo;
	aix.Begin = Ai_Begin;
	aix.Spawn = Ai_Spawn;
	aix.Think = Ai_Think;

	aix.GameRestarted = Ai_GameStarted;

	aix.RegisterItem = Ai_RegisterItem;

	aix.SetDataPointers = Ai_SetDataPointers;

	return &aix;
}
