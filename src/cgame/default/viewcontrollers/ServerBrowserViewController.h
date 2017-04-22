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

#include "TabViewController.h"

/**
 * @file
 * @brief ServerBrowser ViewController.
 */

typedef struct ServerBrowserViewController ServerBrowserViewController;
typedef struct ServerBrowserViewControllerInterface ServerBrowserViewControllerInterface;

/**
 * @brief The ServerBrowserViewController type.
 * @extends TabViewController
 * @ingroup
 */
struct ServerBrowserViewController {

	/**
	 * @brief The superclass.
	 * @private
	 */
	TabViewController tabViewController;

	/**
	 * @brief The interface.
	 * @private
	 */
	ServerBrowserViewControllerInterface *interface;
};

/**
 * @brief The ServerBrowserViewController interface.
 */
struct ServerBrowserViewControllerInterface {

	/**
	 * @brief The superclass interface.
	 */
	TabViewControllerInterface tabViewControllerInterface;
};

/**
 * @fn Class *ServerBrowserViewController::_ServerBrowserViewController(void)
 * @brief The ServerBrowserViewController archetype.
 * @return The ServerBrowserViewController Class.
 * @memberof ServerBrowserViewController
 */
extern Class *_ServerBrowserViewController(void);