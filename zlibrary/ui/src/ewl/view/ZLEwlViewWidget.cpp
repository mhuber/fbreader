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

#include <ewl/Ewl.h>

#include "ZLEwlViewWidget.h"
#include "ZLEwlPaintContext.h"

static void imageReveal(Ewl_Widget *w, void *ev, void *data) {
	((ZLEwlViewWidget*)data)->doPaint();
}

static void updatePoint(ZLEwlViewWidget *viewWidget, int &x, int &y) {
	switch (viewWidget->rotation()) {
		default:
			break;
		case ZLViewWidget::DEGREES90:
		{
			int tmp = x;
			x = viewWidget->height() - y;
			y = tmp;
			break;
		}
		case ZLViewWidget::DEGREES180:
			x = viewWidget->width() - x;
			y = viewWidget->height() - y;
			break;
		case ZLViewWidget::DEGREES270:
		{
			int tmp = x;
			x = y;
			y = viewWidget->width() - tmp;
			break;
		}
	}
}

int ZLEwlViewWidget::width() const {
	Evas_Coord img_w, img_h;

	if(myImage != 0) {
		evas_object_image_size_get(myImage, &img_w, &img_h);
		return img_w;
	}

	return 0;
}

int ZLEwlViewWidget::height() const {
	Evas_Coord img_w, img_h;

	if(myImage != 0) {
		evas_object_image_size_get(myImage, &img_w, &img_h);
		return img_h;
	}

	return 0;
}

ZLEwlViewWidget::ZLEwlViewWidget(ZLApplication *application, Angle initialAngle) : ZLViewWidget(initialAngle) {
	myApplication = application;

	Ewl_Widget *win, *image;

	win = ewl_widget_name_find("main_win");
	if(!win)
		return;

	image = ewl_image_new();
	EWL_IMAGE(image)->image = NULL;
	ewl_object_fill_policy_set(EWL_OBJECT(image), EWL_FLAG_FILL_FILL);
	ewl_container_child_append(EWL_CONTAINER(win), image);
	ewl_callback_append(image, EWL_CALLBACK_REVEAL, imageReveal, this);
	ewl_object_position_request(EWL_OBJECT(image), CURRENT_X(win), CURRENT_Y(win));
	ewl_object_size_request(EWL_OBJECT(image), CURRENT_W(win), CURRENT_H(win));
	ewl_widget_name_set(image, "image");
	ewl_widget_show(image);

	myRepaintBlocked = false;
}

ZLEwlViewWidget::~ZLEwlViewWidget() {
}

void ZLEwlViewWidget::doPaint()	{
	Ewl_Widget *win, *i;
	Evas_Coord img_w, img_h;

	win = ewl_widget_name_find("main_win");
	i = ewl_widget_name_find("image");
	myImage = (Evas_Object*)EWL_IMAGE(i)->image;

	evas_object_image_size_get(myImage, &img_w, &img_h);
	if((img_w != CURRENT_W(win)) || (img_h != CURRENT_H(win))) {
		evas_object_image_size_set(myImage, CURRENT_W(win), CURRENT_H(win));
		evas_object_image_size_get(myImage, &img_w, &img_h);
	}

	int *data;
	data = (int *)evas_object_image_data_get(myImage, 1);
	if(!data)
		return;

	ZLEwlPaintContext &pContext = (ZLEwlPaintContext&)view()->context();
	pContext.setImage(data, width(), height());

	view()->paint();
	myRepaintBlocked = true;
	evas_object_image_data_update_add(myImage, 0, 0, img_w, img_h);
	//	myApplication->refreshWindow();
	myRepaintBlocked = false;
}

void ZLEwlViewWidget::trackStylus(bool track) {
}

void ZLEwlViewWidget::repaint()	{
	if (!myRepaintBlocked) {
		doPaint();
	}
}
