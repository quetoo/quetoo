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

#include "cl_local.h"

/**
 * @brief Clears all volatile view members so that a new scene may be populated.
 */
void Cl_ClearView(void) {

	r_view.num_entities = 0;
	r_view.num_lights = 0;
	r_view.num_particles = 0;
	r_view.num_stains = 0;
	r_view.num_sprites = 0;
	r_view.num_sprite_images = 0;
}

/**
 * @brief Updates the view definition for the pending render frame.
 */
void Cl_UpdateView(void) {

	r_view.ticks = cl.unclamped_time;
	r_view.area_bits = cl.frame.area_bits;

	cls.cgame->UpdateView(&cl.frame);
}

/**
 * @brief
 */
void Cl_InitView(void) {

}
