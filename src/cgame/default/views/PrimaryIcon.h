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

#include <ObjectivelyMVC/Button.h>

/**
 * @file
 * @brief Primary Button.
 */

#define DEFAULT_PRIMARY_ICON_WIDTH 36

typedef struct PrimaryIcon PrimaryIcon;
typedef struct PrimaryIconInterface PrimaryIconInterface;

/**
 * @brief The PrimaryIcon type.
 * @extends Button
 */
struct PrimaryIcon {

	/**
	 * @brief The superclass.
	 * @private
	 */
	Button button;

	/**
	 * @brief The interface.
	 * @private
	 */
	PrimaryIconInterface *interface;

	/**
	 * @brief The icon.
	 **/
	ImageView *imageView;
};

/**
 * @brief The PrimaryIcon interface.
 */
struct PrimaryIconInterface {

	/**
	 * @brief The superclass interface.
	 */
	ButtonInterface buttonInterface;

	/**
	 * @fn PrimaryIcon *PrimaryIcon::initWithFrame(PrimaryIcon *self, const SDL_Rect *frame, const char *icon)
	 * @brief Initializes this PrimaryIcon with the specified frame and style.
	 * @param frame The frame.
	 * @param style The ControlStyle.
	 * @return The initialized PrimaryIcon, or `NULL` on error.
	 * @memberof PrimaryIcon
	 */
	PrimaryIcon *(*initWithFrame)(PrimaryIcon *self, const SDL_Rect *frame, const char *icon);
};

/**
 * @fn Class *PrimaryIcon::_PrimaryIcon(void)
 * @brief The PrimaryIcon archetype.
 * @return The PrimaryIcon Class.
 * @memberof PrimaryIcon
 */
extern Class *_PrimaryIcon(void);