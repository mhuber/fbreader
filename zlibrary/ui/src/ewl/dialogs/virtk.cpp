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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include "Ewl.h"
#include "virtk.h"

typedef struct _layout {
	int x;
	int y;
	char *keys[10][10];
	int size[10][10];
} t_layout;

t_layout *layouts[10];
int layouts_cnt;

t_layout *layout;

typedef struct _virtk_info_struct {
	virtk_handler handler;
	Ewl_Widget *parent;
} virtk_info_struct;

size_t get_char_len(char *str, int len)
{
	unsigned long codepoint;
	unsigned char in_code;
	int expect = 0;

	char *p = str;
	while (*p && len--) {
		in_code = *p++ ;

		if (in_code >= 0xC0) {
			if (in_code < 0xE0) {          /*  U+0080 - U+07FF   */
				expect = 1;
				codepoint = in_code & 0x1F;
			} else if(in_code < 0xF0) {      /*  U+0800 - U+FFFF   */
				expect = 2;
				codepoint = in_code & 0x0F;
			} else if(in_code < 0xF8) {     /* U+10000 - U+10FFFF */
				expect = 3;
				codepoint = in_code & 0x07;
			}
			continue;

		} else if(in_code >= 0x80) {
			--expect;

			if(expect >= 0) {
				codepoint <<= 6;
				codepoint  += in_code & 0x3F;
			}

			if(expect >  0)
				continue;

			expect = 0;

		} else                              /* ASCII, U+0000 - U+007F */
			codepoint = in_code;

		return p - str;
	}

	return 0;
}

/*void free_layouts()
{
	return;
	for(int i =0; i < layouts_cnt; i++) {
		if(layouts[i] == NULL)
			continue;

		for(int y = 0; y < 10; y++)		
			for(int x = 0; x < 10; x++)
				if(layouts[i]->size[y][x])
					free(layouts[i]->keys[y][x]);
		free(layouts[i]);
		layouts[i] = NULL;
	}
	layouts_cnt = 0;
	layout = NULL;
}
*/

void set_layout(int idx)
{
	Ewl_Widget *hbox, *vbox, *label;
	int x, y;
	char *name, *text;

	layout = layouts[idx];

	for(y = 0; y < 11; y++) {		
		asprintf(&name, "hbox_%d", y);
		hbox = ewl_widget_name_find(name);
		free(name);
		if(y >= layout->y) 
			ewl_widget_hide(hbox);
		else
			ewl_widget_show(hbox);

		for(x = 0; x < 11; x++) {

			asprintf(&name, "vbox_%d_%d", x, y);
			vbox = ewl_widget_name_find(name);
			free(name);

			asprintf(&name, "label_%d_%d", x, y);
			label = ewl_widget_name_find(name);
			free(name);

			if(y >= layout->y) {
				ewl_widget_hide(vbox);
				ewl_widget_hide(label);
				continue;
			}
 
			ewl_widget_show(vbox);
			if(x == 0 && y == 0)
				continue;
			
			ewl_widget_show(label);

			if(y == 0) {
				asprintf(&text, "%d", x % 10);
			} else if(x == 0) {
				asprintf(&text, "%d", y % 10);
			} else {
				if(layout->size[y][x]) {
					if(*layout->keys[y][x] == '\n')
						asprintf(&text, "ENT");
					else if(*layout->keys[y][x] == ' ')
						asprintf(&text, "SP");
					else {
						text = (char*)malloc(layout->size[y][x] * sizeof(char));
						memcpy(text, layout->keys[y][x], layout->size[y][x]);
					}
				} else {
					asprintf(&text, " ");
					ewl_widget_hide(label);
				}
			}
			ewl_label_text_set(EWL_LABEL(label), text);
			free(text);
		}
	}
}

void keypress_cb(Ewl_Widget *w, void *event, void *data)
{
	static int i = 0;
	static int first_k;
	int pressed_k;
	char *c;
	Ewl_Widget *win = ewl_widget_name_find("virtk");
	virtk_info_struct *info = (virtk_info_struct *)ewl_widget_data_get(win, (void *)"virtk_info");
	Ewl_Widget *entry = ewl_widget_name_find("virtk_entry");

	Ewl_Event_Key_Up *e = (Ewl_Event_Key_Up*)event;

	const char *k = e->base.keyname;

	if(isdigit(k[0]) && !k[1]) {
		pressed_k = k[0] - '0';
		if(pressed_k == 0)
			pressed_k = 10;

		if(first_k == 0) {
			first_k = pressed_k;
			if(first_k < 1 || first_k > layout->y)
				first_k = 0;
		} else {
			c = layout->keys[first_k][pressed_k];			
			first_k = 0;
			if(c) {
				ewl_text_text_append(&EWL_ENTRY(entry)->text, c);
			}
		}
	} else if(!strcmp(k, "Return")) {
//		ewl_widget_hide(win);
		ewl_widget_destroy(win);
		//free_layouts();
		Ewl_Widget *mwin = ewl_widget_name_find("main_win");
		if(mwin)
			ewl_window_keyboard_grab_set(EWL_WINDOW(mwin), 1);
		(info->handler)(ewl_text_text_get(&EWL_ENTRY(entry)->text));
	} else if(!strcmp(k, "Escape")) {
		if((e->base.modifiers & EWL_KEY_MODIFIER_ALT) || !(ewl_text_length_get(&EWL_ENTRY(entry)->text))) {
//			ewl_widget_hide(win);
			ewl_widget_destroy(win);
			//free_layouts();
			Ewl_Widget *mwin = ewl_widget_name_find("main_win");
			if(mwin)
				ewl_window_keyboard_grab_set(EWL_WINDOW(mwin), 1);
			(info->handler)("");
		}

		first_k = 0;

		ewl_entry_delete_left(EWL_ENTRY(entry));
	} else if(!strcmp(k, "Up") || !strcmp(k, "Down")) {
		if(!strcmp(k, "Up"))
			i++;
		else
			i--;

		if(i < 0)
			i = layouts_cnt - 1;

		i %= layouts_cnt;
		set_layout(i);
	}
}

void read_layouts()
{
	int x, y;
	char *keys = NULL;
	char buf[1024];
	size_t len, buflen = 0;

	FILE *l = fopen("/usr/share/FBReader/themes/virtk_layout", "r");

	if(l == NULL) {
		printf("no layouts found\n");
		exit(0);
	}

	bzero(buf, 1024 * sizeof(char));
	do {
		len = fread(buf, 1, 1024, l);

		if(keys == NULL) {
			keys = (char*)malloc(1 + len * sizeof(char));
			bzero(keys, 1 + len);
		} else {
			int keyslen;
			keyslen = strlen(keys);
			keys = (char*)realloc(keys, 1 + keyslen + len);
			bzero(keys + keyslen, 1 + len);
		}

		memcpy(keys + strlen(keys), buf, len);
		buflen += len;
	} while(!feof(l));

	fclose(l);

	layouts_cnt = 0;

	char *p = keys;
	char *end;
	do {
		while((p - keys) < buflen && *p == '\n')
			++p;

		char *p2 = p;
		int cnt = 0;
		while((p - keys) < buflen && !(*p == '\n' && *(p+1) == '\n')) {		
			p += get_char_len(p, strlen(p));
			cnt++;
		}
		end = p;
		p = p2;

		layout = layouts[layouts_cnt++] = (t_layout *)malloc(sizeof(t_layout));
		
		layout->x = 10;
		layout->y = 1 + (cnt + 9) / 10;
		bzero(layout->keys, 10 * 10 * sizeof(char));

		for(y = 0; p != end && y < layout->y; y++) {	
			for(x = 0; p != end && x < 11; x++) {
				if(x > 0 && y > 0 && strlen(p)) {
					int len = get_char_len(p, strlen(p));
					if(len) {
						layout->keys[y][x] = (char*)malloc(len * sizeof(char) + 1);
						memset(layout->keys[y][x], 0, len + 1);
						memcpy(layout->keys[y][x], p, len);
						layout->size[y][x] = len + 1;
						p += len;
					}
				}
			}
		}
	} while((p - keys < buflen) && (layouts_cnt < 10));

	free(keys);
}

static void virtk_reveal_cb(Ewl_Widget *w, void *ev, void *data) {
	ewl_window_move(EWL_WINDOW(w), (600 - CURRENT_W(w)) / 2, (800 - CURRENT_H(w)) / 2);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
}

static void virtk_realize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 0);
}

static void virtk_unrealize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);
}

Ewl_Widget *init_virtk(Ewl_Widget *parent, char *ltext, virtk_handler handler)
{
	Ewl_Widget *win, *mvbox, *vbox, *hbox, *label, *ebox, *entry;
	char *text;
	int x, y;

/*	win = ewl_widget_name_find("virtk");
	if(win) {
		ewl_window_keyboard_grab_set(EWL_WINDOW(parent), 0);
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);

		entry = ewl_widget_name_find("virtk_entry");
		if(entry) {
			printf("entry clear\n");
//			ewl_widget_hide(entry);
			ewl_text_clear(&EWL_ENTRY(entry)->text);
			ewl_widget_hide(entry);
			ewl_widget_reveal(entry);
			ewl_widget_realize(entry);
			ewl_widget_configure(entry);
			ewl_entry_editable_set(EWL_ENTRY(entry), 1);
			ewl_widget_show(entry);
			ewl_widget_reveal(win);
			ewl_widget_realize(win);
			ewl_widget_configure(win);
			ewl_entry_cursor_position_set(EWL_ENTRY_CURSOR(EWL_ENTRY(entry)->cursor), 1);
		}

		virtk_info_struct *info = (virtk_info_struct *)ewl_widget_data_get(win, (void *)"virtk_info");

		info->handler = handler;
		info->parent = parent;

		set_layout(0);

		return win;
	}
*/
	char *vk_theme = "/usr/share/FBReader/themes/virtktheme.edj";

	virtk_info_struct *info = (virtk_info_struct *)malloc(sizeof(virtk_info_struct));
	info->handler = handler;
	info->parent = parent;

	win = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(win), "EWL_WINDOW");
	ewl_window_name_set(EWL_WINDOW(win), "EWL_WINDOW");
	ewl_window_class_set(EWL_WINDOW(win), "virtk");
	ewl_callback_append(win, EWL_CALLBACK_KEY_UP, keypress_cb, NULL);
	ewl_callback_append(win, EWL_CALLBACK_REVEAL, virtk_reveal_cb, NULL);
	ewl_callback_append(win, EWL_CALLBACK_REALIZE, virtk_realize_cb, NULL);
	ewl_callback_append(win, EWL_CALLBACK_UNREALIZE, virtk_unrealize_cb, NULL);
	ewl_theme_data_str_set(EWL_WIDGET(win), "/window/file", vk_theme);
	ewl_theme_data_str_set(EWL_WIDGET(win), "/window/group", "ewl/oi_window");
	ewl_widget_name_set(win, "virtk");
	ewl_widget_data_set(EWL_WIDGET(win), (void *)"virtk_info", (void *)info);
	ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);
	EWL_EMBED(win)->x = 600;
	EWL_EMBED(win)->y = 0;
	ewl_widget_show(win);

	mvbox = ewl_vbox_new();
	ewl_container_child_append(EWL_CONTAINER(win), mvbox);
	ewl_object_fill_policy_set(EWL_OBJECT(mvbox), EWL_FLAG_FILL_FILL);
//	ewl_object_fill_policy_set(EWL_OBJECT(mvbox), EWL_FLAG_FILL_VSHRINKABLE);
	ewl_widget_show(mvbox);

	ebox = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(mvbox), ebox);
	//ewl_object_fill_policy_set(EWL_OBJECT(ebox), EWL_FLAG_FILL_FILL);
	ewl_object_fill_policy_set(EWL_OBJECT(ebox), EWL_FLAG_FILL_HFILL | EWL_FLAG_FILL_VSHRINKABLE);
	ewl_widget_show(ebox);

	label = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(label), ltext);
	ewl_container_child_append(EWL_CONTAINER(ebox), label);
	ewl_widget_show(label);

	ebox = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(mvbox), ebox);
	//ewl_object_fill_policy_set(EWL_OBJECT(ebox), EWL_FLAG_FILL_FILL);
	ewl_object_fill_policy_set(EWL_OBJECT(ebox), EWL_FLAG_FILL_HFILL | EWL_FLAG_FILL_VSHRINKABLE);
	ewl_widget_show(ebox);

	entry = ewl_entry_new();
	ewl_entry_editable_set(EWL_ENTRY(entry), 1);
	ewl_callback_del(EWL_WIDGET(entry), EWL_CALLBACK_KEY_DOWN, ewl_entry_cb_key_down);
	ewl_container_child_append(EWL_CONTAINER(ebox), entry);
	ewl_theme_data_str_set(EWL_WIDGET(entry), "/entry/file", vk_theme);
	ewl_theme_data_str_set(EWL_WIDGET(entry), "/entry/group", "ewl/dlg_entry");
	ewl_theme_data_str_set(EWL_WIDGET(entry), "/entry/cursor/file", vk_theme);
	ewl_theme_data_str_set(EWL_WIDGET(entry), "/entry/cursor/group", "ewl/dlg_entry/cursor");
	ewl_widget_name_set(entry, "virtk_entry");
	ewl_widget_show(entry);


	for(y = 0; y < 11; y++) {		
		hbox = ewl_hbox_new();
		ewl_container_child_append(EWL_CONTAINER(mvbox), hbox);
//		ewl_object_fill_policy_set(EWL_OBJECT(hbox), EWL_FLAG_FILL_FILL);
		ewl_object_fill_policy_set(EWL_OBJECT(hbox), EWL_FLAG_FILL_HFILL | EWL_FLAG_FILL_VSHRINKABLE);
		asprintf(&text, "hbox_%d", y);
		ewl_widget_name_set(hbox, text);
		free(text);

		for(x = 0; x < 11; x++) {
			vbox = ewl_vbox_new();
			ewl_container_child_append(EWL_CONTAINER(hbox), vbox);
			ewl_object_fill_policy_set(EWL_OBJECT(vbox), EWL_FLAG_FILL_FILL);

			asprintf(&text, "vbox_%d_%d", x, y);
			ewl_widget_name_set(vbox, text);
			free(text);

			if(x == 0 && y == 0)
				continue;

			label = ewl_label_new();
			ewl_container_child_append(EWL_CONTAINER(vbox), label);
			ewl_theme_data_str_set(EWL_WIDGET(label), "/label/file", vk_theme);
			if((!x && y) || (x && !y)) {
				ewl_theme_data_str_set(EWL_WIDGET(label), "/label/group", "ewl/oi_key_label_2");
				ewl_theme_data_str_set(EWL_WIDGET(label), "/label/textpart", "ewl/oi_key_label_2/text");
			} else {
				ewl_theme_data_str_set(EWL_WIDGET(label), "/label/group", "ewl/oi_key_label");
				ewl_theme_data_str_set(EWL_WIDGET(label), "/label/textpart", "ewl/oi_key_label/text");
			}
			asprintf(&text, "label_%d_%d", x, y);
			ewl_widget_name_set(label, text);
			free(text);
		}
	}

	if(layouts_cnt == 0) {
		read_layouts();
	}
	set_layout(0);

	return win;
}
