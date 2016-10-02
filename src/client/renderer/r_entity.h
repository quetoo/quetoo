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

#ifndef __R_ENTITY_H__
#define __R_ENTITY_H__

#include "r_types.h"

const r_entity_t *R_AddEntity(const r_entity_t *e);
const r_entity_t *R_AddLinkedEntity(const r_entity_t *parent, const r_model_t *model, const char *tag_name);
void R_SetMatrixForEntity(r_entity_t *e);

#ifdef __R_LOCAL_H__
void R_RotateForEntity(const r_entity_t *e);
void R_CullEntities(void *data);
void R_DrawEntities(void);
#endif /* __R_LOCAL_H__ */

#endif /* __R_ENTITY_H__ */
