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

#ifndef __NET_MESSAGE_H__
#define __NET_MESSAGE_H__

#include "common.h"

/**
 * @brief Delta compression flags for pm_state_t.
 */
#define PS_PM_TYPE				0x1
#define PS_PM_ORIGIN			0x2
#define PS_PM_VELOCITY			0x4
#define PS_PM_FLAGS				0x8
#define PS_PM_TIME				0x10
#define PS_PM_GRAVITY			0x20
#define PS_PM_VIEW_OFFSET		0x40
#define PS_PM_VIEW_ANGLES		0x80
#define PS_PM_KICK_ANGLES		0x100
#define PS_PM_DELTA_ANGLES		0x200

/**
 * @brief Delta compression flags for user_cmd_t.
 */
#define CMD_ANGLE1 				0x1
#define CMD_ANGLE2 				0x2
#define CMD_ANGLE3 				0x4
#define CMD_FORWARD				0x8
#define CMD_RIGHT				0x10
#define CMD_UP					0x20
#define CMD_BUTTONS				0x40

/**
 * @brief These flags indicate which fields in a given entity_state_t must be
 * written or read for delta compression from one snapshot to the next.
 */
#define U_ORIGIN				0x1 // origin
#define U_TERMINATION			0x2 // beams
#define U_ANGLES				0x4 // angles
#define U_ANIMATIONS			0x8 // animation frames
#define U_EVENT					0x10 // single-frame events
#define U_EFFECTS				0x20 // bit masked effects
#define U_TRAIL					0x40 // enumerated trails
#define U_MODELS				0x80 // models (primary and linked)
#define U_CLIENT				0x100 // offset into client skins array
#define U_SOUND					0x200 // looped sounds
#define U_SOLID					0x400 // solid type
#define U_BOUNDS				0x800 // encoded bounding box
#define U_REMOVE				0x1000 // remove this entity, don't add it

/**
 * @brief These flags indicate which fields a given sound packet will contain.
 */
#define S_ATTEN					0x1
#define S_ORIGIN				0x2
#define S_ENTITY				0x4

/**
 * @brief Message writing and reading facilities.
 */
void Net_WriteData(mem_buf_t *msg, const void *data, size_t len);
void Net_WriteChar(mem_buf_t *msg, const int32_t c);
void Net_WriteByte(mem_buf_t *msg, const int32_t c);
void Net_WriteShort(mem_buf_t *msg, const int32_t c);
void Net_WriteLong(mem_buf_t *msg, const int32_t c);
void Net_WriteString(mem_buf_t *msg, const char *s);
void Net_WriteVector(mem_buf_t *msg, const vec_t f);
void Net_WritePosition(mem_buf_t *msg, const vec3_t pos);
void Net_WriteAngle(mem_buf_t *msg, const vec_t f);
void Net_WriteAngles(mem_buf_t *msg, const vec3_t angles);
void Net_WriteDir(mem_buf_t *msg, const vec3_t dir);
void Net_WriteDeltaMoveCmd(mem_buf_t *msg, const pm_cmd_t *from, const pm_cmd_t *to);
void Net_WriteDeltaPlayerState(mem_buf_t *msg, const player_state_t *from, const player_state_t *to);
void Net_WriteDeltaEntity(mem_buf_t *msg, const entity_state_t *from, const entity_state_t *to, _Bool force);

void Net_BeginReading(mem_buf_t *msg);
void Net_ReadData(mem_buf_t *msg, void *data, size_t len);
int32_t Net_ReadChar(mem_buf_t *msg);
int32_t Net_ReadByte(mem_buf_t *msg);
int32_t Net_ReadShort(mem_buf_t *msg);
int32_t Net_ReadLong(mem_buf_t *msg);
char *Net_ReadString(mem_buf_t *msg);
char *Net_ReadStringLine(mem_buf_t *msg);
vec_t Net_ReadVector(mem_buf_t *msg);
void Net_ReadPosition(mem_buf_t *msg, vec3_t pos);
vec_t Net_ReadAngle(mem_buf_t *msg);
void Net_ReadAngles(mem_buf_t *msg, vec3_t angles);
void Net_ReadDir(mem_buf_t *msg, vec3_t vector);
void Net_ReadDeltaMoveCmd(mem_buf_t *msg, const pm_cmd_t *from, pm_cmd_t *to);
void Net_ReadDeltaPlayerState(mem_buf_t *msg, const player_state_t *from, player_state_t *to);
void Net_ReadDeltaEntity(mem_buf_t *msg, const entity_state_t *from, entity_state_t *to,
		uint16_t number, uint16_t bits);

#endif /* __NET_MESSAGE_H__ */
