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

#ifndef __ZLEWLVIEWWIDGET_H__
#define __ZLEWLVIEWWIDGET_H__

#include "../../../../core/src/view/ZLViewWidget.h"
#include <ZLApplication.h>

extern "C" {
#include <xcb/xcb.h>
#include <xcb/shm.h>
}

class ZLEwlViewWidget : public ZLViewWidget {

public:
	ZLEwlViewWidget(ZLApplication *application, ZLView::Angle initialAngle);
	~ZLEwlViewWidget();

	int width() const;
	int height() const;
	void doPaint();
	void invertRegion(int x0, int y0, int x1, int y1, bool flush);

	void setScrollbarEnabled(ZLView::Direction, bool) {} 
	void setScrollbarPlacement(ZLView::Direction, bool) {}
	void setScrollbarParameters(ZLView::Direction, size_t, size_t, size_t) {}


private:
	void trackStylus(bool track);
	void repaint();

private:
	xcb_gcontext_t		gc;
	xcb_gcontext_t		bgcolor;
	unsigned int pal_[4];

	ZLApplication *myApplication;
	bool myRepaintBlocked;
};

#endif /* __ZLEWLVIEWWIDGET_H__ */
