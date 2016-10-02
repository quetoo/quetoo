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

#include "cg_local.h"

#include "CvarSelect.h"

#define _Class _CvarSelect

#pragma mark - View

/**
 * @see View::updateBindings(View *)
 */
static void updateBindings(View *self) {

	super(View, self, updateBindings);

	CvarSelect *this = (CvarSelect *) self;

	if (this->expectsStringValue) {
		const Array *options = (Array *) this->select.options;
		for (size_t i = 0; i < options->count; i++) {
			const Option *option = $(options, objectAtIndex, i);
			if (strcmp(option->title->text, this->var->string) == 0) {
				$((Select *) this, selectOptionWithValue, option->value);
				break;
			}
		}
	} else {
		$((Select *) this, selectOptionWithValue, (ident) (intptr_t) this->var->integer);
	}
}

#pragma mark - Select

/**
 * @see Select::addOption(Select *, const char *, ident)
 */
static void addOption(Select *self, const char *title, ident value) {

	super(Select, self, addOption, title, value);

	$((View *) self, updateBindings);
}

#pragma mark - CvarSelect

/**
 * @brief SelectDelegate callback.
 */
static void didSelectOption(Select *Select, Option *option) {

	const CvarSelect *this = (CvarSelect *) Select;

	if (this->expectsStringValue) {
		cgi.CvarSet(this->var->name, option->title->text);
	} else {
		cgi.CvarSetValue(this->var->name, (int32_t) (intptr_t) option->value);
	}
}

/**
 * @fn CvarSelect *CvarSelect::initWithVariable(CvarSelect *self, cvar_t *var)
 *
 * @memberof CvarSelect
 */
static CvarSelect *initWithVariable(CvarSelect *self, cvar_t *var) {

	self = (CvarSelect *) super(Select, self, initWithFrame, NULL, ControlStyleDefault);
	if (self) {

		self->var = var;
		assert(self->var);

		Select *this = (Select *) self;

		this->delegate.didSelectOption = didSelectOption;
	}

	return self;
}

/**
 * @fn CvarSelect *CvarSelect::initWithVariabeName(CvarSelect *self, const char (name)
 *
 * @memberof CvarSelect
 */
static CvarSelect *initWithVariableName(CvarSelect *self, const char *name) {
	return $(self, initWithVariable, cgi.CvarGet(name));
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

	((ViewInterface *) clazz->interface)->updateBindings = updateBindings;

	((SelectInterface *) clazz->interface)->addOption = addOption;

	((CvarSelectInterface *) clazz->interface)->initWithVariable = initWithVariable;
	((CvarSelectInterface *) clazz->interface)->initWithVariableName = initWithVariableName;
}

Class _CvarSelect = {
	.name = "CvarSelect",
	.superclass = &_Select,
	.instanceSize = sizeof(CvarSelect),
	.interfaceOffset = offsetof(CvarSelect, interface),
	.interfaceSize = sizeof(CvarSelectInterface),
	.initialize = initialize,
};

#undef _Class

