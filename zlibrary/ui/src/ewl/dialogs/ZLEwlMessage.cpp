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

#include "ZLEwlMessage.h"
#include <iostream>
#include <cstring>

extern void redraw_text();

static void message_reveal_cb(Ewl_Widget *w, void *ev, void *data) {
	ewl_window_move(EWL_WINDOW(w), (600 - CURRENT_W(w)) / 2, (800 - CURRENT_H(w)) / 2);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
}

static void message_realize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 0);
}

static void message_unrealize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);
}

static void message_keyhandler(Ewl_Widget *w, void *ev, void *data)
{
	Ewl_Event_Key_Down *e;
	Ewl_Widget *message, *dialog;
	char *s;
	int n;

	e = (Ewl_Event_Key_Down*)ev;

	if(!strcmp(e->base.keyname, "Escape") 
			|| !strcmp(e->base.keyname, "Return")) {
		ewl_widget_hide(w);
		ewl_main_quit();
//		if(data)
//			redraw_text();
	}
}

Ewl_Widget *init_message(char *text, bool master)
{
	Ewl_Widget *w, *label, *message, *message_hbox;

	w = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(w), "Message");
	ewl_window_name_set(EWL_WINDOW(w), "EWL_WINDOW");
	ewl_window_class_set(EWL_WINDOW(w), "Message");
	ewl_widget_name_set(w, "message_win");
	ewl_callback_append(w, EWL_CALLBACK_KEY_UP, message_keyhandler, (void*)master);
	ewl_callback_append(w, EWL_CALLBACK_REVEAL, message_reveal_cb, NULL);
	ewl_callback_append(w, EWL_CALLBACK_REALIZE, message_realize_cb, NULL);
	ewl_callback_append(w, EWL_CALLBACK_UNREALIZE, message_unrealize_cb, NULL);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
	EWL_EMBED(w)->x = 600;
	EWL_EMBED(w)->y = 0;
	ewl_widget_show(w);

	message_hbox = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(w), message_hbox);
	ewl_widget_show(message_hbox);

	label = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(label), text);
	ewl_container_child_append(EWL_CONTAINER(message_hbox), label);
	ewl_widget_show(label);

	ewl_widget_focus_send(w);

	return w;
}
