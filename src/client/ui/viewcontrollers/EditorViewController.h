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

#include "MenuViewController.h"

/**
 * @file
 *
 * @brief The EditorViewController.
 */

typedef struct EditorViewController EditorViewController;
typedef struct EditorViewControllerInterface EditorViewControllerInterface;

/**
 * @brief The EditorViewController type.
 * @extends MenuViewController
 * @ingroup ViewControllers
 */
struct EditorViewController {

	/**
	 * @brief The superclass.
	 * @private
	 */
	MenuViewController menuViewController;

	/**
	 * @brief The interface.
	 * @private
	 */
	EditorViewControllerInterface *interface;

	/**
	 * @brief The material the player is looking at.
	 */
	r_material_t *material;

	/**
	 * @brief The bump slider.
	 */
	Slider *bumpSlider;

	/**
	 * @brief The hardness slider.
	 */
	Slider *hardnessSlider;

	/**
	 * @brief The specular slider.
	 */
	Slider *specularSlider;

	/**
	 * @brief The parallax slider.
	 */
	Slider *parallaxSlider;
};


/**
 * @brief The EditorViewController interface.
 */
struct EditorViewControllerInterface {

	/**
	 * @brief The superclass interface.
	 */
	NavigationViewControllerInterface navigationViewControllerInterface;

	/**
	 * @fn EditorViewController *EditorViewController::init(EditorViewController *self)
	 * @brief Initializes this ViewController.
	 * @return The initialized EditorViewController, or `NULL` on error.
	 * @memberof EditorViewController
	 */
	EditorViewController *(*init)(EditorViewController *self);
};

/**
 * @fn Class *EditorViewController::_EditorViewController(void)
 * @brief The EditorViewController archetype.
 * @return The EditorViewController Class.
 * @memberof EditorViewController
 */
extern Class *_EditorViewController(void);
