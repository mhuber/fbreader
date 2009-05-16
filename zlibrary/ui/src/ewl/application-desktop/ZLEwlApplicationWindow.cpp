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

extern bool _fbreader_closed;

void ZLEwlDialogManager::createApplicationWindow(ZLApplication *application) const {
	myWindow = EWL_WINDOW((new ZLEwlApplicationWindow(application))->getMainWindow());
}

static const std::string OPTIONS = "Options";

ZLEwlApplicationWindow::ZLEwlApplicationWindow(ZLApplication *application) : ZLDesktopApplicationWindow(application) {
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
	ZLEwlViewWidget *viewWidget = new ZLEwlViewWidget(&application(), (ZLView::Angle)application().AngleStateOption.value());
	return viewWidget;
}

void ZLEwlApplicationWindow::close() {
	_fbreader_closed = true;
}

void ZLEwlApplicationWindow::setCaption(const std::string &caption) {
}
