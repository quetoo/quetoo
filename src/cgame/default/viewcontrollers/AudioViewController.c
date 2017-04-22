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

#include "AudioViewController.h"

#define _Class _AudioViewController

#pragma mark - Actions

/**
 * @brief ActionFunction for Apply Button.
 */
static void applyAction(Control *control, const SDL_Event *event, ident sender, ident data) {
	cgi.Cbuf("s_restart\n");
}

#pragma mark - ViewController

/**
 * @see ViewController::loadView(ViewController *)
 */
static void loadView(ViewController *self) {

	super(ViewController, self, loadView);

	TabViewController *this = (TabViewController *) self;

	StackView *columns = $(alloc(StackView), initWithFrame, NULL);

	columns->spacing = DEFAULT_PANEL_SPACING;

	columns->axis = StackViewAxisHorizontal;
	columns->distribution = StackViewDistributionFillEqually;

	columns->view.autoresizingMask = ViewAutoresizingFill;

	{
		StackView *column = $(alloc(StackView), initWithFrame, NULL);

		column->spacing = DEFAULT_PANEL_SPACING;

		{
			Box *box = $(alloc(Box), initWithFrame, NULL);
			$(box->label, setText, "Volumes");

			box->view.autoresizingMask |= ViewAutoresizingWidth;

			StackView *stackView = $(alloc(StackView), initWithFrame, NULL);

			Cgui_CvarSliderInput((View *) stackView, "Effect volume", "s_volume", 0.0, 1.0, 0.1);
			Cgui_CvarSliderInput((View *) stackView, "Music volume", "s_music_volume", 0.0, 1.0, 0.1);

			Cgui_CvarCheckboxInput((View *) stackView, "Ambient sound", "s_ambient");

			$((View *) box, addSubview, (View *) stackView);
			release(stackView);

			$((View *) column, addSubview, (View *) box);
			release(box);
		}

		$((View *) columns, addSubview, (View *) column);
		release(column);
	}

	{
		StackView *column = $(alloc(StackView), initWithFrame, NULL);

		column->spacing = DEFAULT_PANEL_SPACING;

		{
			Box *box = $(alloc(Box), initWithFrame, NULL);
			$(box->label, setText, "Options");

			box->view.autoresizingMask |= ViewAutoresizingWidth;

			StackView *stackView = $(alloc(StackView), initWithFrame, NULL);

			Cgui_CvarCheckboxInput((View *) stackView, "Swap stereo", "s_reverse");

			$((View *) box, addSubview, (View *) stackView);
			release(stackView);

			$((View *) column, addSubview, (View *) box);
			release(box);
		}

		{
			Box *box = $(alloc(Box), initWithFrame, NULL);
			$(box->label, setText, "Sounds");

			box->view.autoresizingMask |= ViewAutoresizingWidth;

			StackView *stackView = $(alloc(StackView), initWithFrame, NULL);

			Cgui_CvarCheckboxInput((View *) stackView, "Hit sound", cg_hit_sound->name);

			$((View *) box, addSubview, (View *) stackView);
			release(stackView);

			$((View *) column, addSubview, (View *) box);
			release(box);
		}

		$((View *) columns, addSubview, (View *) column);
		release(column);
	}

	$((View *) this->panel->contentView, addSubview, (View *) columns);
	release(columns);

	this->panel->accessoryView->view.hidden = false;

	Cgui_Button((View *) this->panel->accessoryView, "Apply", applyAction, self, NULL);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

	((ViewControllerInterface *) clazz->def->interface)->loadView = loadView;
}

/**
 * @fn Class *AudioViewController::_AudioViewController(void)
 * @memberof AudioViewController
 */
Class *_AudioViewController(void) {
	static Class clazz;
	static Once once;

	do_once(&once, {
		clazz.name = "AudioViewController";
		clazz.superclass = _TabViewController();
		clazz.instanceSize = sizeof(AudioViewController);
		clazz.interfaceOffset = offsetof(AudioViewController, interface);
		clazz.interfaceSize = sizeof(AudioViewControllerInterface);
		clazz.initialize = initialize;
	});

	return &clazz;
}

#undef _Class