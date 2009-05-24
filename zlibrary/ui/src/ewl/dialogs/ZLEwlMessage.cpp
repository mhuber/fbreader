/*
 * Copyright (C) 2009 Alexander Kerner <lunohod@openinkpot.org>
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

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "ZLEwlMessage.h"
#include <Ecore.h>
#include <Ecore_X.h>
#include <Ecore_Evas.h>
#include <Edje.h>

using namespace std;

extern void ee_init();

static void die(const char* fmt, ...)
{
   va_list ap;
   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
   exit(EXIT_FAILURE);
}

static int exit_handler(void* param, int ev_type, void* event)
{
   ecore_main_loop_quit();
   return 1;
}

static void main_win_close_handler(Ecore_Evas* main_win)
{
   ecore_main_loop_quit();
}


static void main_win_resize_handler(Ecore_Evas* main_win)
{
   Evas* canvas = ecore_evas_get(main_win);
   int w, h;
   evas_output_size_get(canvas, &w, &h);

   Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "main_canvas_edje");
   evas_object_resize(main_canvas_edje, w, h);
}

static void main_win_signal_handler(void* param, Evas_Object* o, const char* emission, const char* source)
{
   printf("%s -> %s\n", source, emission);
}

static void main_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

	if(param) {
		void (*handler)(Evas_Object*, char *) = (void(*)(Evas_Object*, char*))param;
		handler(o, ev->keyname);
	}
	if(!strcmp(ev->keyname, "Escape"))
		ecore_main_loop_quit();
}

void show_message(char *text, void *handler)
{
	ee_init();

	Ecore_Evas* root = ecore_evas_software_x11_new(0, 0, 0, 0, 0, 0);
	ecore_evas_title_set(root, "r");
	ecore_evas_name_class_set(root, "r", "r");

	Ecore_Evas* main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
	ecore_evas_title_set(main_win, "MB");
	ecore_evas_name_class_set(main_win, "MB", "MB");

	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_window_get(main_win),
			ecore_evas_software_x11_window_get(root));

	Evas* main_canvas = ecore_evas_get(main_win);

	ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

	Evas_Object* main_canvas_edje = edje_object_add(main_canvas);
	evas_object_name_set(main_canvas_edje, "main_canvas_edje");
	edje_object_file_set(main_canvas_edje, "/usr/share/FBReader/themes/messagebox.edj", "message");
	edje_object_signal_callback_add(main_canvas_edje, "*", "*", main_win_signal_handler, NULL);
	edje_object_part_text_set(main_canvas_edje, "text", text);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_resize(main_canvas_edje, 600, 800);

	Evas_Coord x, y, w, h;
	const Evas_Object *t = edje_object_part_object_get(main_canvas_edje, "text");
	evas_object_geometry_get(t, &x, &y, &w, &h);

	ecore_evas_resize(main_win, w+20, h+20);
	evas_object_resize(main_canvas_edje, w+20, h+20);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_show(main_canvas_edje);

	evas_object_focus_set(main_canvas_edje, 1);
	evas_object_event_callback_add(main_canvas_edje,
			EVAS_CALLBACK_KEY_DOWN,
			&main_win_key_handler,
			handler);

	ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

	ecore_evas_show(main_win);

	ecore_main_loop_iterate();

	ecore_main_loop_begin();

	ecore_evas_hide(main_win);
	ecore_evas_free(main_win);
	ecore_evas_hide(root);
	ecore_evas_free(root);
}

// entry

#define MAXDIGITS	6

struct entry_number {
	char *text;
	long number;
	int cnt;
};

static void entry_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
    Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;

	struct entry_number *number = (struct entry_number*)param;

	if(isdigit(ev->keyname[3]) && !ev->keyname[4] && number->cnt < MAXDIGITS) {
		if(number->number > 0)
			number->number *= 10;
		else
			number->number = 0;

		number->number += ev->keyname[3] - '0';
		if(number->number > 0)
			number->cnt++;

		char *t;
		if(number->number > 0)
			asprintf(&t, "%s: %-*d", number->text, MAXDIGITS, number->number);
		else
			asprintf(&t, "%s: %-*c", number->text, MAXDIGITS, 0);
		edje_object_part_text_set(o, "entrytext", t);
		free(t);
	}
	if(!strcmp(ev->keyname, "Escape")) {
		if(number->cnt > 0) {
			number->cnt--;
			number->number /= 10;

			char *t;
			if(number->number > 0)
				asprintf(&t, "%s: %-*d", number->text, MAXDIGITS, number->number);
			else
				asprintf(&t, "%s: %-*c", number->text, MAXDIGITS, 0);
			edje_object_part_text_set(o, "entrytext", t);
			free(t);
		} else {
			number->number = -1;
			ecore_main_loop_quit();
		}
	}
	if(!strcmp(ev->keyname, "Return"))
		ecore_main_loop_quit();
}

long read_number(char *text)
{
	ee_init();

	struct entry_number number;
	number.text = text;
	number.number = -1;
	number.cnt = 0;

	Ecore_Evas* root = ecore_evas_software_x11_new(0, 0, 0, 0, 0, 0);
	ecore_evas_title_set(root, "r");
	ecore_evas_name_class_set(root, "r", "r");

	Ecore_Evas* main_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);
	ecore_evas_title_set(main_win, "EB");
	ecore_evas_name_class_set(main_win, "EB", "EB");

	ecore_x_icccm_transient_for_set(
			ecore_evas_software_x11_window_get(main_win),
			ecore_evas_software_x11_window_get(root));

	Evas* main_canvas = ecore_evas_get(main_win);

	ecore_evas_callback_delete_request_set(main_win, main_win_close_handler);

	Evas_Object* main_canvas_edje = edje_object_add(main_canvas);
	evas_object_name_set(main_canvas_edje, "main_canvas_edje");
	edje_object_file_set(main_canvas_edje, "/usr/share/FBReader/themes/entrybox.edj", "entrybox");
	edje_object_signal_callback_add(main_canvas_edje, "*", "*", main_win_signal_handler, NULL);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_resize(main_canvas_edje, 600, 800);

	char *t;
	asprintf(&t, "%s: %-*d", text, MAXDIGITS, 999999);
	edje_object_part_text_set(main_canvas_edje, "entrytext", t);
	free(t);

	Evas_Coord x, y, w, h, x2, y2, w2, h2;
	evas_object_geometry_get(
			edje_object_part_object_get(main_canvas_edje, "entrytext"),
			&x, &y, &w, &h);

	asprintf(&t, "%s: %-*c", text, MAXDIGITS, 0);
	edje_object_part_text_set(main_canvas_edje, "entrytext", t);
	free(t);

	ecore_evas_resize(main_win, w+40, h+20);
	evas_object_resize(main_canvas_edje, w+40, h+20);
	evas_object_move(main_canvas_edje, 0, 0);
	evas_object_show(main_canvas_edje);

	evas_object_focus_set(main_canvas_edje, 1);
	evas_object_event_callback_add(main_canvas_edje,
			EVAS_CALLBACK_KEY_DOWN,
			&entry_win_key_handler,
			(void*)&number);

	ecore_evas_callback_resize_set(main_win, main_win_resize_handler);

	ecore_evas_show(main_win);

	ecore_main_loop_iterate();

	ecore_main_loop_begin();

	ecore_evas_hide(main_win);
	ecore_evas_free(main_win);
	ecore_evas_hide(root);
	ecore_evas_free(root);

	return number.number;
}
