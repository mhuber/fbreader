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

/* FIXME */
#define BUFSIZE 512

/* FIXME */
const int header_h = 49;
const int footer_h = 49;


static Ecore_Evas *lcb_win, *fcb_win;

static void cb_rcb_destroy();

static bool alt_modifier = false;

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
    char buf[BUFSIZE];
    if(total_pages < 2)
        *buf = 0;
    else
        snprintf(buf, BUFSIZE, "%d/%d", cur_page + 1, total_pages);

    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "lcb_footer");

    edje_object_part_text_set(footer, "text", buf);
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

	if(l->items.at(item_num).item_handler != NULL)
		l->items.at(item_num).item_handler(item_num, is_alt);
	else
		l->item_handler(item_num, is_alt);
}


static void lcb_win_resize_handler(Ecore_Evas* main_win)
{
	Evas* canvas = ecore_evas_get(main_win);
	int w, h;
	evas_output_size_get(canvas, &w, &h);

	Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "main_canvas_edje");
	evas_object_resize(main_canvas_edje, w, h);

	Evas_Object *bg = evas_object_name_find(canvas, "bg");
	evas_object_resize(bg, w, h);

	Evas_Object* header = evas_object_name_find(canvas, "lcb_header");
	evas_object_move(header, 0, 0);
	evas_object_resize(header, w/2, header_h);

	Evas_Object* footer = evas_object_name_find(canvas, "lcb_footer");
	evas_object_move(footer, 0, h - footer_h);
	evas_object_resize(footer, w/2, footer_h);

	header = evas_object_name_find(canvas, "rcb_header");
	if(header) {
		evas_object_move(header, w/2, 0);
		evas_object_resize(header, w/2, header_h);
	}

	footer = evas_object_name_find(canvas, "rcb_footer");
	if(footer) {
		evas_object_move(footer, w/2, h - footer_h);
		evas_object_resize(footer, w/2, footer_h);
	}

	Evas_Object* choicebox = evas_object_name_find(canvas, SETTINGS_LEFT_NAME);
	if(choicebox) {
		evas_object_resize(choicebox, w/2, h - header_h - footer_h);
		evas_object_move(choicebox, 0, header_h);
	}

	choicebox = evas_object_name_find(canvas, SETTINGS_RIGHT_NAME);
	if(choicebox) {
		evas_object_resize(choicebox, w/2, h - header_h - footer_h);
		evas_object_move(choicebox, w/2, header_h);
	}
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

	if(l->destroy_handler != NULL)
		l->destroy_handler();

	delete l;
	olists.pop_back();

	if(olists.size() > 0)
		cb_lcb_redraw();
	else
		ecore_main_loop_quit();
}

static void lcb_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
	fprintf(stderr, "kn: %s, k: %s, s: %s, c: %s\n", ev->keyname, ev->key, ev->string, ev->compose);

	const char *k = ev->key;

	Evas_Object* r = evas_object_name_find(e, SETTINGS_LEFT_NAME);

    if(!strcmp(ev->keyname, "Up") || !strcmp(ev->keyname, "Prior"))
		choicebox_prev(r);
    if(!strcmp(ev->keyname, "Down") || !strcmp(ev->keyname, "Next"))
		choicebox_next(r);
	if(!strcmp(k, "Left"))
		choicebox_prevpage(r);
	if(!strcmp(k, "Right"))
		choicebox_nextpage(r);
	if(!strncmp("KP_", k, 3) && isdigit(k[3]) && k[3] != '0' && !k[4])
		choicebox_activate_nth_visible(r, k[3]-'1', alt_modifier);
	if(!strcmp(k, "Return"))
		choicebox_activate_current(r, alt_modifier);
	if(!strcmp(k, "Escape"))
		cb_lcb_destroy();
	if(!strcmp(k, "Alt_L")) {
		alt_modifier = true;
		edje_object_signal_emit(evas_object_name_find(e, "lcb_footer"), alt_modifier ? "alt_on" : "alt_off", "");
	}
	if(!strcmp(k, "space")) {
		alt_modifier = !alt_modifier;
		edje_object_signal_emit(evas_object_name_find(e, "lcb_footer"), alt_modifier ? "alt_on" : "alt_off", "");
	}
}

static void lcb_win_key_up_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
	if(!strcmp(ev->key, "Alt_L"))
		alt_modifier = false;
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
	Evas_Object* header = evas_object_name_find(canvas, "lcb_header");
	edje_object_part_text_set(header, "text", olists.empty() ? "" : olists.back()->name.c_str());
	choicebox_set_size(choicebox, l->items.size());
	choicebox_invalidate_interval(choicebox, 0, l->items.size());
	if(l->items.size() > 0)
		choicebox_set_selection(choicebox, 0);
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

		ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, exit_handler, NULL);

		_init = 1;
	}
}

void cb_lcb_new()
{
	cb_ee_init();


	lcb_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);

	ecore_evas_title_set(lcb_win, "LCB");
	ecore_evas_name_class_set(lcb_win, "LCB", "LCB");

	Evas* main_canvas = ecore_evas_get(lcb_win);

	ecore_evas_callback_delete_request_set(lcb_win, lcb_win_close_handler);

	Evas_Object* bg = evas_object_rectangle_add(main_canvas);
	evas_object_name_set(bg, "bg");
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_move(bg, 0, 0);
	evas_object_resize(bg, 600, 800);
	evas_object_show(bg);

	Evas_Object* header = edje_object_add(main_canvas);
	evas_object_name_set(header, "lcb_header");
	edje_object_file_set(header, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"header");
	edje_object_part_text_set(header, "text", olists.empty() ? "" : olists.back()->name.c_str());
	evas_object_move(header, 0, 0);
	evas_object_resize(header, 300, header_h);
	evas_object_show(header);

	Evas_Object* footer = edje_object_add(main_canvas);
	evas_object_name_set(footer, "lcb_footer");
	edje_object_file_set(footer, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"footer");
	evas_object_move(footer, 0, 800 - footer_h);
	evas_object_resize(footer, 300, footer_h);
	evas_object_show(footer);

	Evas_Object* choicebox = choicebox_new(main_canvas, "/usr/share/echoicebox/echoicebox.edj", "settings-left",
			lcb_handler, lcb_draw_handler, lcb_page_updated_handler, (void*)NULL);
	choicebox_set_size(choicebox, olists.empty() ? 0 : olists.back()->items.size());
	evas_object_name_set(choicebox, SETTINGS_LEFT_NAME);
	evas_object_resize(choicebox, 300, 800 - header_h - footer_h);
	evas_object_move(choicebox, 0, header_h);
	evas_object_show(choicebox);

	evas_object_focus_set(choicebox, true);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_DOWN,
			&lcb_win_key_handler,
			NULL);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_UP,
			&lcb_win_key_up_handler,
			NULL);

	ecore_evas_callback_resize_set(lcb_win, lcb_win_resize_handler);

	ecore_evas_show(lcb_win);

	ecore_main_loop_begin();

	ecore_evas_hide(lcb_win);
	ecore_evas_free(lcb_win);
	lcb_win = NULL;
}

// rcb
static void rcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param)
{
    char buf[BUFSIZE];
    if(total_pages < 2)
        *buf = 0;
    else
        snprintf(buf, BUFSIZE, "%d/%d", cur_page + 1, total_pages);

    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "rcb_footer");

    edje_object_part_text_set(footer, "text", buf);
}

static void rcb_draw_handler(Evas_Object* choicebox,
		Evas_Object* item,
		int item_num,
		int page_position,
		void* param)
{
	char foo[256];

	if(!vlist)
		return;

	if(item_num >= vlist->values.size())
		return;

	cb_item_value *iv = &vlist->values.at(item_num);
	edje_object_part_text_set(item, "text", iv->text.c_str());
	edje_object_part_text_set(item, "title", "");
	edje_object_part_text_set(item, "value", "");

	fprintf(stderr, "rcd_draw_handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
			choicebox, item, item_num, page_position, param);
}

static void rcb_handler(Evas_Object* choicebox,
		int item_num,
		bool is_alt,
		void* param)
{
	printf("rcb_handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
			choicebox, item_num, is_alt, param);

	if(!vlist)
		return;

	vlist->item_handler(item_num, is_alt);

	cb_rcb_destroy();
}

static void rcb_win_signal_handler(void* param, Evas_Object* o, const char* emission, const char* source)
{
	printf("%s -> %s\n", source, emission);
}

static void cb_rcb_destroy()
{
	Evas* e = ecore_evas_get(lcb_win);

	Evas_Object *o = evas_object_name_find(e, SETTINGS_RIGHT_NAME);
	evas_object_hide(o);
	evas_object_del(o);

	o = evas_object_name_find(e, "rcb_header");
	evas_object_hide(o);
	evas_object_del(o);

	o = evas_object_name_find(e, "rcb_footer");
	evas_object_hide(o);
	evas_object_del(o);

	evas_object_focus_set(evas_object_name_find(e, SETTINGS_LEFT_NAME), true);
	if(vlist) {
		delete vlist;
		vlist = NULL;
	}
}

static void rcb_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
	fprintf(stderr, "kn: %s, k: %s, s: %s, c: %s\n", ev->keyname, ev->key, ev->string, ev->compose);

	const char *k = ev->key;

	Evas_Object* r = evas_object_name_find(e, SETTINGS_RIGHT_NAME);


    if(!strcmp(ev->keyname, "Up") || !strcmp(ev->keyname, "Prior"))
		choicebox_prev(r);
    if(!strcmp(ev->keyname, "Down") || !strcmp(ev->keyname, "Next"))
		choicebox_next(r);
	if(!strcmp(k, "Left"))
		choicebox_prevpage(r);
	if(!strcmp(k, "Right"))
		choicebox_nextpage(r);
	if(!strncmp("KP_", k, 3) && isdigit(k[3]) && k[3] != '0' && !k[4])
		choicebox_activate_nth_visible(r, k[3]-'1', alt_modifier);
	if(!strcmp(k, "Return"))
		choicebox_activate_current(r, alt_modifier);
	if(!strcmp(k, "Escape"))
		cb_rcb_destroy();
	if(!strcmp(k, "Alt_L"))
		alt_modifier = true;
	if(!strcmp(k, "space")) {
		alt_modifier = !alt_modifier;
	}
}

static void rcb_win_key_up_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
	if(!strcmp(ev->key, "Alt_L"))
		alt_modifier = false;
}

void cb_rcb_new()
{
	Evas* main_canvas = ecore_evas_get(lcb_win);
	int w, h;
	evas_output_size_get(main_canvas, &w, &h);
	w /= 2;

	Evas_Object* header = edje_object_add(main_canvas);
	evas_object_name_set(header, "rcb_header");
	edje_object_file_set(header, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"header");
	edje_object_part_text_set(header, "text", vlist->name.c_str());
	evas_object_move(header, w, 0);
	evas_object_resize(header, w, header_h);
	evas_object_show(header);

	Evas_Object* footer = edje_object_add(main_canvas);
	evas_object_name_set(footer, "rcb_footer");
	edje_object_file_set(footer, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"footer");
	evas_object_move(footer, w, h - footer_h);
	evas_object_resize(footer, w, footer_h);
	evas_object_show(footer);

	Evas_Object* choicebox = choicebox_new(main_canvas, "/usr/share/echoicebox/echoicebox.edj", "settings-right",
			rcb_handler, rcb_draw_handler, rcb_page_updated_handler, (void*)NULL);
	choicebox_set_size(choicebox, vlist->values.size());
	evas_object_name_set(choicebox, SETTINGS_RIGHT_NAME);
	evas_object_resize(choicebox, w, h - header_h - footer_h);
	evas_object_move(choicebox, w, header_h);
	evas_object_show(choicebox);

	evas_object_focus_set(choicebox, true);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_DOWN,
			&rcb_win_key_handler,
			NULL);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_UP,
			&rcb_win_key_up_handler,
			NULL);
}

// fcb
static void fcb_win_close_handler(Ecore_Evas* main_win)
{
	fprintf(stderr, "main_win_close_handler\n");
	ecore_main_loop_quit();
}

static void fcb_page_updated_handler(Evas_Object* choicebox,
		int cur_page,
		int total_pages,
		void* param)
{
    char buf[BUFSIZE];
    if(total_pages < 2)
        *buf = 0;
    else
        snprintf(buf, BUFSIZE, "%d/%d", cur_page + 1, total_pages);

    Evas* canvas = evas_object_evas_get(choicebox);
    Evas_Object* footer = evas_object_name_find(canvas, "footer");

    edje_object_part_text_set(footer, "text", buf);
}

static void fcb_draw_handler(Evas_Object* choicebox,
		Evas_Object* item,
		int item_num,
		int page_position,
		void* param)
{
	cb_list *l = (cb_list*)param;
	
	if(l->items.empty())
		return;

	edje_object_part_text_set(item, "text", l->items.at(item_num).c_str());

	fprintf(stderr, "handle: choicebox: %p, item: %p, item_num: %d, page_position: %d, param: %p\n",
			choicebox, item, item_num, page_position, param);
}

static void fcb_handler(Evas_Object* choicebox,
		int item_num,
		bool is_alt,
		void* param)
{
	printf("handle: choicebox: %p, item_num: %d, is_alt: %d, param: %p\n",
			choicebox, item_num, is_alt, param);

	cb_list *l = (cb_list *)param;
	if(l->item_handler(item_num, is_alt) != 0)
		ecore_main_loop_quit();
}


static void fcb_win_resize_handler(Ecore_Evas* main_win)
{
	Evas* canvas = ecore_evas_get(main_win);
	int w, h;
	evas_output_size_get(canvas, &w, &h);

	Evas_Object* main_canvas_edje = evas_object_name_find(canvas, "main_canvas_edje");
	evas_object_resize(main_canvas_edje, w, h);

	Evas_Object *bg = evas_object_name_find(canvas, "bg");
	evas_object_resize(bg, w, h);

	Evas_Object* header = evas_object_name_find(canvas, "header");
	evas_object_move(header, 0, 0);
	evas_object_resize(header, w, header_h);

	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	if(choicebox) {
		evas_object_resize(choicebox, w, h - header_h - footer_h);
		evas_object_move(choicebox, 0, header_h);
	}

	Evas_Object* footer = evas_object_name_find(canvas, "footer");
	evas_object_move(footer, 0, h - footer_h);
	evas_object_resize(footer, w, footer_h);
}

static void fcb_win_signal_handler(void* param, Evas_Object* o, const char* emission, const char* source)
{
	printf("%s -> %s\n", source, emission);
}

static void cb_fcb_destroy()
{
//	if(l->destroy_handler != NULL)
//		l->destroy_handler();

	ecore_main_loop_quit();
}

static void fcb_win_key_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Down* ev = (Evas_Event_Key_Down*)event_info;
	fprintf(stderr, "kn: %s, k: %s, s: %s, c: %s\n", ev->keyname, ev->key, ev->string, ev->compose);

	const char *k = ev->key;

	Evas_Object* r = evas_object_name_find(e, "cb_full");

    if(!strcmp(ev->keyname, "Up") || !strcmp(ev->keyname, "Prior"))
		choicebox_prev(r);
    if(!strcmp(ev->keyname, "Down") || !strcmp(ev->keyname, "Next"))
		choicebox_next(r);
	if(!strcmp(k, "Left"))
		choicebox_prevpage(r);
	if(!strcmp(k, "Right"))
		choicebox_nextpage(r);
	if(!strncmp("KP_", k, 3) && isdigit(k[3]) && k[3] != '0' && !k[4])
		choicebox_activate_nth_visible(r, k[3]-'1', alt_modifier);
	if(!strcmp(k, "Return"))
		choicebox_activate_current(r, alt_modifier);
	if(!strcmp(k, "Escape"))
		cb_fcb_destroy();
	if(!strcmp(k, "Alt_L")) {
		alt_modifier = true;
		edje_object_signal_emit(evas_object_name_find(e, "footer"), alt_modifier ? "alt_on" : "alt_off", "");
	}
	if(!strcmp(k, "space")) {
		alt_modifier = !alt_modifier;

		edje_object_signal_emit(evas_object_name_find(e, "footer"), alt_modifier ? "alt_on" : "alt_off", "");
	}
}

static void fcb_win_key_up_handler(void* param, Evas* e, Evas_Object* o, void* event_info)
{
	Evas_Event_Key_Up* ev = (Evas_Event_Key_Up*)event_info;
	if(!strcmp(ev->key, "Alt_L")) {
		alt_modifier = false;
		edje_object_signal_emit(evas_object_name_find(e, "footer"), alt_modifier ? "alt_on" : "alt_off", "");
	}
}

void cb_fcb_invalidate(int idx)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	choicebox_invalidate_item(choicebox, idx);
}

void cb_fcb_invalidate_interval(int start, int end)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	choicebox_invalidate_interval(choicebox, start, end);
}

void cb_fcb_redraw(int newsize)
{
	Evas* canvas = ecore_evas_get(fcb_win);
	Evas_Object* choicebox = evas_object_name_find(canvas, "cb_full");
	if(newsize >= 0) {
		choicebox_set_size(choicebox, newsize);
		choicebox_invalidate_interval(choicebox, 0, newsize);
	}
	if(newsize > 0)
		choicebox_set_selection(choicebox, 0);
}

void cb_fcb_new(cb_list *list)
{
	cb_ee_init();

	fcb_win = ecore_evas_software_x11_new(0, 0, 0, 0, 600, 800);

	ecore_evas_title_set(fcb_win, "FCB");
	ecore_evas_name_class_set(fcb_win, "FCB", "FCB");

	Evas* main_canvas = ecore_evas_get(fcb_win);

	ecore_evas_callback_delete_request_set(fcb_win, fcb_win_close_handler);

	Evas_Object* bg = evas_object_rectangle_add(main_canvas);
	evas_object_name_set(bg, "bg");
	evas_object_color_set(bg, 255, 255, 255, 255);
	evas_object_move(bg, 0, 0);
	evas_object_resize(bg, 600, 800);
	evas_object_show(bg);

	Evas_Object* header = edje_object_add(main_canvas);
	evas_object_name_set(header, "header");
	edje_object_file_set(header, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"header");
	edje_object_part_text_set(header, "text", list->name.c_str());
	evas_object_move(header, 0, 0);
	evas_object_resize(header, 600, header_h);
	evas_object_show(header);

	Evas_Object* footer = edje_object_add(main_canvas);
	evas_object_name_set(footer, "footer");
	edje_object_file_set(footer, "/usr/share/FBReader/themes/cb_header_footer.edj",
			"footer");
	evas_object_move(footer, 0, 800 - footer_h);
	evas_object_resize(footer, 600, footer_h);
	evas_object_show(footer);

	Evas_Object* choicebox = choicebox_new(main_canvas, "/usr/share/echoicebox/echoicebox.edj", "full",
			fcb_handler, fcb_draw_handler, fcb_page_updated_handler, list);
	choicebox_set_size(choicebox, list->items.size());
	evas_object_name_set(choicebox, "cb_full");
	evas_object_resize(choicebox, 600, 800 - header_h - footer_h);
	evas_object_move(choicebox, 0, header_h);
	evas_object_show(choicebox);

	evas_object_focus_set(choicebox, true);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_DOWN,
			&fcb_win_key_handler,
			NULL);
	evas_object_event_callback_add(choicebox,
			EVAS_CALLBACK_KEY_UP,
			&fcb_win_key_up_handler,
			NULL);

	ecore_evas_callback_resize_set(fcb_win, fcb_win_resize_handler);

	ecore_evas_show(fcb_win);

	ecore_main_loop_begin();

	ecore_evas_hide(fcb_win);
	ecore_evas_free(fcb_win);
	fcb_win = NULL;
}
