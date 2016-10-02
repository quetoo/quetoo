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

#include <ObjectivelyMVC/ImageView.h>
#include <ObjectivelyMVC/View.h>

#include "cg_types.h"

/**
 * @file
 *
 * @brief A View capable of rendering an r_mesh_model_t.
 */

typedef struct PlayerModelView PlayerModelView;
typedef struct PlayerModelViewInterface PlayerModelViewInterface;

/**
 * @brief The PlayerModelView type.
 *
 * @extends View
 */
struct PlayerModelView {
	
	/**
	 * @brief The parent.
	 *
	 * @private
	 */
	View view;
	
	/**
	 * @brief The typed interface.
	 *
	 * @private
	 */
	PlayerModelViewInterface *interface;

	/**
	 * @brief The client information.
	 */
	cl_client_info_t client;

	/**
	 * @brief The entity stubs.
	 */
	r_entity_t head, torso, legs, weapon;

	/**
	 * @brief The entity animations.
	 */
	cl_entity_animation_t animation1, animation2;

	ImageView *iconView;
};

/**
 * @brief The PlayerModelView interface.
 */
struct PlayerModelViewInterface {
	
	/**
	 * @brief The parent interface.
	 */
	ViewInterface viewInterface;

	/**
	 * @fn void PlayerModelView::animate(PlayerModelView *self)
	 *
	 * @brief Animates the model.
	 *
	 * @memberof PlayerModelView
	 */
	void (*animate)(PlayerModelView *self);
	
	/**
	 * @fn PlayerModelView *PlayerModelView::initWithFrame(PlayerModelView *self)
	 *
	 * @brief Initializes this PlayerModelView.
	 *
	 * @param frame The frame.
	 *
	 * @return The initialized PlayerModelView, or `NULL` on error.
	 *
	 * @memberof PlayerModelView
	 */
	PlayerModelView *(*initWithFrame)(PlayerModelView *self, const SDL_Rect *frame);
};

/**
 * @brief The PlayerModelView Class.
 */
extern Class _PlayerModelView;

