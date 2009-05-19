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

#include <ZLibrary.h>

#include "ZLEwlDialogs.h"
#include "ZLEwlChoicebox_new.h"

#include <iostream>
#include <cstdlib>
#include <cstring>

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

extern "C" {
#include <echoicebox.h>
}

#define SETTINGS_LEFT_NAME "settings_left"
#define SETTINGS_RIGHT_NAME "settings_right"

static Ecore_Evas *lcb_win, *rcb_win;

static int exit_handler(void* param, int ev_type, void* event)
{
	fprintf(stderr, "exit_handler\n");
	ecore_main_loop_quit();
	return 1;
}

static void lcb_win_close_handler(Ecore_Evas* main_win)
{
	fprintf(stderr, "main_win_close_handler\n");
	ecore_main_loop_quit();
}

static void lcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param)
{
}

static void lcb_draw_handler(Evas_Object* choicebox,
		Evas_Object* item,
		int item_num,
		int page_position,
		void* param)
{
	char foo[256];

	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	if(item_num >= l->items.size())
		return;

	cb_olist_item *i = &l->items.at(item_num);
	if(i->type == ITEM_SUBMENU || i->type == ITEM_TEXT) {
		edje_object_part_text_set(item, "text", i->name.c_str());
		edje_object_part_text_set(item, "title", "");
		edje_object_part_text_set(item, "value", "");
	} else if(i->type == ITEM_OPTION) {
		edje_object_part_text_set(item, "text", "");
		edje_object_part_text_set(item, "title", i->name.c_str());

		if(i->values.empty())
			edje_object_part_text_set(item, "value",  i->current_value.text.c_str());
		else if(i->values.size() <= 3) {
			string s;
			for(int z = 0; z < i->values.size(); z++)
				if(z != i->curval_idx) {
					s += "  <inactive>";
					s += i->values.at(z).text;
					s += "</inactive>";
				} else
					s += "  " + i->values.at(z).text;

				edje_object_part_text_set(item, "value", s.c_str());
		}
	}

	fprintf(stderr, "handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
			choicebox, item, item_num, page_position, param);
}


static void footer_draw_handler(Evas_Object* choicebox,
		Evas_Object* footer,
		int a,
		int b,
		void* param)
{
	char foo[256];
	sprintf(foo, "%d/%d", a, b);
	edje_object_part_text_set(footer, "choicebox/footer/text", foo);
}


static void lcb_handler(Evas_Object* choicebox,
		int item_num,
		bool is_alt,
		void* param)
{
	printf("handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
			choicebox, item_num, is_alt, param);

	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l || !l->item_handler)
		return;

	l->item_handler(item_num);
}


static void lcb_win_resize_handler(Ecore_Evas* main_win)
{
	Evas* canvas = ecore_evas_get(main_win);
	int w, h;
	evas_output_size_get(canvas, &w, &h);

	Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "main_canvas_edje");
	evas_object_resize(main_canvas_edje, w, h);

	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	evas_object_resize(choicebox, w, h);
	evas_object_move(choicebox, 0, 0);
}

static void lcb_win_signal_handler(void* param, Evas_Object* o, const char* emission, const char* source)
{
	printf("%s -> %s\n", source, emission);
}

void cb_lcb_invalidate(int idx)
{
	Evas* canvas = ecore_evas_get(lcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	choicebox_invalidate_item(choicebox, idx);
}

static void cb_lcb_destroy()
{
	if(olists.empty())
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	delete l;
	olists.pop_back();

	if(olists.size() > 0)
		cb_lcb_redraw();
	else {
		//	evas_object_hide(evas_object_name_find(e, "main_canvas_edje"));
		//	evas_object_del(evas_object_name_find(e, "main_canvas_edje"));
		//	evas_free(ecore_evas_get(mw));

		ecore_main_loop_quit();
		ecore_evas_free(lcb_win);
	}
}

static void lcb_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
	fprintf(stderr, "kn: %s, k: %s, s: %s, c: %s\n", ev->keyname, ev->key, ev->string, ev->compose);

	const char *k = ev->key;

	Evas_Object* r = evas_object_name_find(e, SETTINGS_LEFT_NAME);

	if(!strcmp(k, "Up"))
		choicebox_prev(r);
	if(!strcmp(k, "Down"))
		choicebox_next(r);
	if(!strcmp(k, "Left"))
		choicebox_prevpage(r);
	if(!strcmp(k, "Right"))
		choicebox_nextpage(r);
	if(!strncmp("KP_", k, 3) && isdigit(k[3]) && !k[4])
		choicebox_activate_nth_visible(r, k[3]-'0', 0);
	if(!strcmp(k, "Return"))
		choicebox_activate_current(r, 0);
	if(!strcmp(k, "Escape")) {
		cb_lcb_destroy();
	}
}

void cb_lcb_redraw()
{
	if(olists.size() < 1)
		return;

	cb_olist *l = olists.back();
	if(!l)
		return;

	Evas* canvas = ecore_evas_get(lcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	choicebox_set_size(choicebox, l->items.size());
	choicebox_invalidate_interval(choicebox, 0, l->items.size());
}

static void cb_ee_init()
{
	static int _init = 0;
	if(!_init) {
		if(!evas_init())
			return;
		if(!ecore_init())
			return;
		if(!ecore_evas_init())
			return;
		if(!edje_init())
			return;

		_init = 1;
	}
}

void cb_lcb_new()
{
	cb_ee_init();

	ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_handler, NULL);

	lcb_win = ecore_evas_software_x11_new(0, 0, 0, 0, 300, 800);

	ecore_evas_title_set(lcb_win, "LCB");
	ecore_evas_name_class_set(lcb_win, "LCB", "LCB");

	Evas* main_canvas = ecore_evas_get(lcb_win);

	ecore_evas_callback_delete_request_set(lcb_win, lcb_win_close_handler);

	Evas_Object* bg = evas_object_rectangle_add(main_canvas);
	evas_object_name_set(bg, "bg");
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_move(bg, 0, 0);
	evas_object_resize(bg, 300, 800);
	evas_object_show(bg);

	Evas_Object* choicebox = choicebox_new(main_canvas, "/usr/local/share/echoicebox/echoicebox.edj", "settings-left",
			lcb_handler, lcb_draw_handler, lcb_page_updated_handler, (void*)NULL);
	choicebox_set_size(choicebox, olists.empty() ? 0 : olists.back()->items.size());
	evas_object_name_set(choicebox, SETTINGS_LEFT_NAME);
	evas_object_resize(choicebox, 300, 800);
	evas_object_move(choicebox, 0, 0);
	evas_object_show(choicebox);

	evas_object_focus_set(choicebox, true);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_DOWN,
			&lcb_win_key_handler,
			NULL);

	ecore_evas_callback_resize_set(lcb_win, lcb_win_resize_handler);

	ecore_evas_show(lcb_win);

	ecore_main_loop_begin();
}
