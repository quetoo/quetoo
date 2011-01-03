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

#include "client.h"


/*
 * S_AllocChannel
 */
static int S_AllocChannel(void){
	int i;

	for(i = 0; i < MAX_CHANNELS; i++){
		if(!s_env.channels[i].sample)
			return i;
	}

	return -1;
}


/*
 * S_FreeChannel
 */
void S_FreeChannel(int c){

	memset(&s_env.channels[c], 0, sizeof(s_env.channels[0]));
}


#define DISTANCE_SCALE 0.15

/*
 * S_SpatializeChannel
 *
 * Set distance and stereo panning for the specified channel.
 */
void S_SpatializeChannel(s_channel_t *ch){
	entity_state_t *ent;
	vec3_t origin, delta;
	int c;
	float dist, dot, angle;

	c = (int)((ptrdiff_t)(ch - s_env.channels));

	if(ch->entnum != -1){  // update the channel origin
		ent = &cl.entities[ch->entnum].current;
		VectorCopy(ent->origin, ch->org);
	}

	VectorCopy(ch->org, origin);
	origin[2] = r_view.origin[2];

	VectorSubtract(origin, r_view.origin, delta);
	dist = VectorNormalize(delta) * DISTANCE_SCALE * pow(ch->atten, 1.5);

	if(dist > 255.0)  // clamp to max
		dist = 255.0;

	if(dist > 10.0){  // resolve stereo panning
		dot = DotProduct(r_view.right, delta);
		angle = acos(dot) * 180.0 / M_PI - 90.0;

		angle = (int)(360.0 - angle) % 360;
	}
	else
		angle = 0.0;

	Mix_SetPosition(c, (int)angle, (int)dist);
}


/*
 * S_PlaySample
 */
void S_PlaySample(const vec3_t org, int entnum, s_sample_t *sample, int atten){
	s_channel_t *ch;
	int i;

	if(!s_env.initialized)
		return;

	if(!sample)
		return;

	if(sample->name[0] == '*')
		sample = S_LoadModelSample(&cl.entities[entnum].current, sample->name);

	if(!sample->chunk)
		return;

	if((i = S_AllocChannel()) == -1)
		return;

	ch = &s_env.channels[i];

	if(org){  // positioned sound
		VectorCopy(org, ch->org);
		ch->entnum = -1;
	}
	else  // entity sound
		ch->entnum = entnum;

	ch->atten = atten;
	ch->sample = sample;

	S_SpatializeChannel(ch);

	Mix_PlayChannel(i, ch->sample->chunk, 0);
}


/*
 * S_LoopSample
 */
void S_LoopSample(const vec3_t org, s_sample_t *sample){
	s_channel_t *ch;
	vec3_t delta;
	int i;

	if(!sample || !sample->chunk)
		return;

	ch = NULL;

	for(i = 0; i < MAX_CHANNELS; i++){  // find existing loop sound

		if(s_env.channels[i].entnum != -1)
			continue;

		if(s_env.channels[i].sample == sample){

			VectorSubtract(s_env.channels[i].org, org, delta);

			if(VectorLength(delta) < 255.0){
				ch = &s_env.channels[i];
				break;
			}
		}
	}

	if(ch){  // update existing loop sample
		ch->count++;

		VectorMix(ch->org, org, 1.0 / ch->count, ch->org);
	}
	else {  // or allocate a new one

		if((i = S_AllocChannel()) == -1)
			return;

		ch = &s_env.channels[i];

		VectorCopy(org, ch->org);
		ch->entnum = -1;
		ch->count = 1;
		ch->atten = ATTN_IDLE;
		ch->sample = sample;

		Mix_PlayChannel(i, ch->sample->chunk, 0);
	}

	S_SpatializeChannel(ch);
}


/*
 * S_StartLocalSample
 */
void S_StartLocalSample(const char *name){
	s_sample_t *sample;

	if(!s_env.initialized)
		return;

	sample = S_LoadSample(name);

	if(!sample){
		Com_Warn("S_StartLocalSample: Failed to load %s.\n", name);
		return;
	}

	S_PlaySample(NULL, cl.playernum + 1, sample, ATTN_NONE);
}
