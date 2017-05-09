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

#include "InfoViewController.h"

#include "CreditsViewController.h"

#define _Class _InfoViewController

#pragma mark - Object

static void dealloc(Object *self) {

	InfoViewController *this = (InfoViewController *) self;

	release(this->tabViewController);

	super(Object, self, dealloc);
}

#pragma mark - ViewController

/**
 * @see ViewController::loadView(ViewController *)
 */
static void loadView(ViewController *self) {

	super(ViewController, self, loadView);

	InfoViewController *this = (InfoViewController *) self;

	this->menuViewController.panel->stackView.view.padding.top = 0;
	this->menuViewController.panel->stackView.view.padding.right = 0;
	this->menuViewController.panel->stackView.view.padding.bottom = 0;
	this->menuViewController.panel->stackView.view.padding.left = 0;

	this->menuViewController.panel->stackView.view.zIndex = 100;

	this->menuViewController.panel->contentView->view.clipsSubviews = true;

	// Setup the TabViewController

	this->tabViewController = $(alloc(TabViewController), init);

	// Tab buttons

	{
		{
			CreditsViewController *viewController = $((CreditsViewController *) _alloc(_CreditsViewController()), init);

			$((ViewController *) this->tabViewController, addChildViewController, (ViewController *) viewController);

			release(viewController);
		}
	}

	$((ViewController *) this->tabViewController, moveToParentViewController, self);
}

#pragma mark - Class lifecycle

/**
 * @see Class::initialize(Class *)
 */
static void initialize(Class *clazz) {

	((ObjectInterface *) clazz->def->interface)->dealloc = dealloc;

	((ViewControllerInterface *) clazz->def->interface)->loadView = loadView;
}

/**
 * @fn Class *InfoViewController::_InfoViewController(void)
 * @memberof InfoViewController
 */
Class *_InfoViewController(void) {
	static Class clazz;
	static Once once;

	do_once(&once, {
		clazz.name = "InfoViewController";
		clazz.superclass = _MenuViewController();
		clazz.instanceSize = sizeof(InfoViewController);
		clazz.interfaceOffset = offsetof(InfoViewController, interface);
		clazz.interfaceSize = sizeof(InfoViewControllerInterface);
		clazz.initialize = initialize;
	});

	return &clazz;
}

#undef _Class
