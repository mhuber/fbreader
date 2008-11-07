/*
 * Copyright (C) 2008 Alexander Kerner <lunohod@openinkpot.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */


#include <ZLibrary.h>

#include "ZLEwlApplicationWindow.h"

#include "../dialogs/ZLEwlDialogManager.h"
#include "../view/ZLEwlViewWidget.h"
#include "../util/ZLEwlUtil.h"
#include "../../../../core/src/dialogs/ZLOptionView.h"

void ZLEwlDialogManager::createApplicationWindow(ZLApplication *application) const {
	myWindow = EWL_WINDOW((new ZLEwlApplicationWindow(application))->getMainWindow());
}

static void applicationQuit(Ewl_Widget *w, void *ev, void *data) {
	((ZLEwlApplicationWindow*)data)->application().closeView();
	ewl_main_quit();
}

static void deleteWindow(Ewl_Widget *w, void *ev, void *data) {
	ewl_widget_destroy(w);
}

static void handleKeyEvent(Ewl_Widget *w, void *ev, void *data) {
	((ZLEwlApplicationWindow*)data)->handleKeyEventSlot(ev);
}

static const std::string OPTIONS = "Options";

ZLEwlApplicationWindow::ZLEwlApplicationWindow(ZLApplication *application) : ZLDesktopApplicationWindow(application) {
	ewl_theme_theme_set((ZLibrary::ApplicationDirectory() + "/themes/oitheme.edj").c_str());
	myMainWindow = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(myMainWindow), "FBReader");
	ewl_window_class_set(EWL_WINDOW(myMainWindow), "fbreader");
	ewl_window_name_set(EWL_WINDOW(myMainWindow), "fbreader");
	ewl_theme_data_str_set(EWL_WIDGET(myMainWindow), "/window/group", "ewl/blank");
	ewl_object_fill_policy_set(EWL_OBJECT(myMainWindow), EWL_FLAG_FILL_ALL);
	ewl_object_size_request(EWL_OBJECT(myMainWindow), 0, 0);
	EWL_EMBED(myMainWindow)->x = 599;
	EWL_EMBED(myMainWindow)->y = 799;
	ewl_callback_append(myMainWindow, EWL_CALLBACK_KEY_UP, handleKeyEvent, this);
	ewl_callback_append(myMainWindow, EWL_CALLBACK_DELETE_WINDOW, deleteWindow, this);
	ewl_callback_append(myMainWindow, EWL_CALLBACK_DESTROY, applicationQuit, this);
	ewl_widget_name_set(myMainWindow, "main_win");
	ewl_window_lower(EWL_WINDOW(myMainWindow));
	ewl_window_keyboard_grab_set(EWL_WINDOW(myMainWindow), 1);
	ewl_widget_show(myMainWindow);
}

void ZLEwlApplicationWindow::init() {
	ZLDesktopApplicationWindow::init();
	switch (myWindowStateOption.value()) {
		case NORMAL:
			break;
		case FULLSCREEN:
			setFullscreen(true);
			break;
		case MAXIMIZED:
			break;
	}
}

ZLEwlApplicationWindow::~ZLEwlApplicationWindow() {
}

void ZLEwlApplicationWindow::handleKeyEventSlot(void *ev) {
	Ewl_Event_Key_Up *e;
	e = (Ewl_Event_Key_Up *)ev;

	if(e->base.modifiers & EWL_KEY_MODIFIER_ALT)
		application().doActionByKey((std::string("Alt+") + e->base.keyname).c_str());
	else
		application().doActionByKey(e->base.keyname);
}

void ZLEwlApplicationWindow::setFullscreen(bool fullscreen) {
	if (fullscreen == isFullscreen()) {
		return;
	}

	if (fullscreen) {
	} else {
	}
}

bool ZLEwlApplicationWindow::isFullscreen() const {
	return true;
}

ZLViewWidget *ZLEwlApplicationWindow::createViewWidget() {
	ZLEwlViewWidget *viewWidget = new ZLEwlViewWidget(&application(), (ZLViewWidget::Angle)application().AngleStateOption.value());
	return viewWidget;
}

void ZLEwlApplicationWindow::close() {
	ewl_main_quit();
}

void ZLEwlApplicationWindow::setCaption(const std::string &caption) {
	ewl_window_title_set(EWL_WINDOW(myMainWindow), caption.c_str()); 
}
