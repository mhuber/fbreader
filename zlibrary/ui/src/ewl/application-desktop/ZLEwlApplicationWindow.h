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

#ifndef __ZLEWLAPPLICATIONWINDOW_H__
#define __ZLEWLAPPLICATIONWINDOW_H__

#include  <ewl/Ewl.h>

#include <vector>
#include <map>

#include "../../../../core/src/desktop/application/ZLDesktopApplicationWindow.h"

class ZLOptionView;

class ZLEwlApplicationWindow : public ZLDesktopApplicationWindow { 

public:
	ZLEwlApplicationWindow(ZLApplication *application);
	~ZLEwlApplicationWindow();

private:
	ZLViewWidget *createViewWidget();
	void init();
	void close();

	void setCaption(const std::string &caption);

	bool isFullscreen() const;
	void setFullscreen(bool fullscreen);

	void addToolbarItem(shared_ptr<ZLToolbar::Item>) {} 
	void setToggleButtonState(const ZLToolbar::ToggleButtonItem&) {}
	void processAllEvents() {}

private:
	void grabAllKeys(bool grab) { }
	//void addToolbarItem(ZLApplication::Toolbar::ItemPtr item) { }

public:
	void handleKeyEventSlot(void *ev);

	Ewl_Widget *getMainWindow() { return myMainWindow; }

private:
	Ewl_Widget *myMainWindow;
};

#endif /* __ZLEWLAPPLICATIONWINDOW_H__ */
