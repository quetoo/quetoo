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

#include <ObjectivelyMVC/TextView.h>

#include "cg_types.h"

/**
 * @file
 *
 * @brief A TextView bound to a cvar_t.
 */

typedef struct CvarTextView CvarTextView;
typedef struct CvarTextViewInterface CvarTextViewInterface;

/**
 * @brief The CvarTextView type.
 *
 * @extends TextView
 */
struct CvarTextView {
	
	/**
	 * @brief The parent.
	 *
	 * @private
	 */
	TextView textView;
	
	/**
	 * @brief The typed interface.
	 *
	 * @private
	 */
	CvarTextViewInterface *interface;

	/**
	 * @brief The variable.
	 */
	cvar_t *var;
};

/**
 * @brief The CvarTextView interface.
 */
struct CvarTextViewInterface {
	
	/**
	 * @brief The parent interface.
	 */
	TextViewInterface textViewInterface;
	
	/**
	 * @fn CvarTextView *CvarTextView::initWithVariable(CvarTextView *self, cvar_t *var)
	 *
	 * @brief Initializes this TextView with the given variable.
	 *
	 * @param var The variable.
	 *
	 * @return The initialized CvarTextView, or `NULL` on error.
	 *
	 * @memberof CvarTextView
	 */
	CvarTextView *(*initWithVariable)(CvarTextView *self, cvar_t *var);
};

/**
 * @brief The CvarTextView Class.
 */
extern Class _CvarTextView;

