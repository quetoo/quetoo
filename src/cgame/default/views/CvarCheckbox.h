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

#include <ObjectivelyMVC/Checkbox.h>

#include "cg_types.h"

/**
 * @file
 *
 * @brief A Checkbox bound to a cvar_t.
 */

typedef struct CvarCheckbox CvarCheckbox;
typedef struct CvarCheckboxInterface CvarCheckboxInterface;

/**
 * @brief The CvarCheckbox type.
 *
 * @extends Checkbox
 */
struct CvarCheckbox {
	
	/**
	 * @brief The parent.
	 *
	 * @private
	 */
	Checkbox checkbox;
	
	/**
	 * @brief The typed interface.
	 *
	 * @private
	 */
	CvarCheckboxInterface *interface;

	/**
	 * @brief The variable.
	 */
	cvar_t *var;
};

/**
 * @brief The CvarCheckbox interface.
 */
struct CvarCheckboxInterface {
	
	/**
	 * @brief The parent interface.
	 */
	CheckboxInterface checkboxInterface;
	
	/**
	 * @fn CvarCheckbox *CvarCheckbox::initWithVariable(CvarCheckbox *self, cvar_t *var)
	 *
	 * @brief Initializes this Checkbox with the given variable.
	 *
	 * @param var The variable.
	 *
	 * @return The initialized CvarCheckbox, or `NULL` on error.
	 *
	 * @memberof CvarCheckbox
	 */
	CvarCheckbox *(*initWithVariable)(CvarCheckbox *self, cvar_t *var);
};

/**
 * @brief The CvarCheckbox Class.
 */
extern Class _CvarCheckbox;

