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

#include "csg.h"
#include "polylib.h"

csg_brush_t *AllocBrush(int32_t num_sides);
void FreeBrush(csg_brush_t *brush);
void FreeBrushes(csg_brush_t *brushes);
size_t CountBrushes(const csg_brush_t *brushes);
csg_brush_t *CopyBrush(const csg_brush_t *brush);
float BrushVolume(csg_brush_t *brush);
csg_brush_t *BrushFromBounds(const vec3_t mins, const vec3_t maxs);
int32_t BrushOnPlaneSide(const csg_brush_t *brush, int32_t plane_num);
int32_t BrushOnPlaneSideSplits(const csg_brush_t *brush, int32_t plane_num, int32_t *num_splits);
void SplitBrush(const csg_brush_t *brush, int32_t plane_num, csg_brush_t **front, csg_brush_t **back);
