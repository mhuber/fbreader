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

#include "ZLEwlDialogs.h"
#include "ZLEwlChoicebox.h"

#include <ewl/Ewl.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "../util/ZLEwlUtil.h"
#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"
#include "../../../../../zlibrary/text/src/view/ZLTextStyle.h"

static void ZLEwlGotoPageDialog_reveal(Ewl_Widget *w, void *ev, void *data) {
	ewl_window_move(EWL_WINDOW(w), (600 - CURRENT_W(w)) / 2, (800 - CURRENT_H(w)) / 2);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
}

static void ZLEwlGotoPageDialog_realize(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 0);
}

static void ZLEwlGotoPageDialog_unrealize(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);
}


static void ZLEwlGotoPageDialog_window_close_cb(Ewl_Widget *w, void *ev, void *data)
{
	ewl_widget_destroy(w);
	manual_update(true);
}

static void ZLEwlGotoPageDialog_key_up_cb(Ewl_Widget *w, void *ev, void *data)
{
	Ewl_Event_Key_Down *e;
	Ewl_Widget *entry, *dialog;
	char *s;
	int n;

	busy();

	e = (Ewl_Event_Key_Down*)ev;
	w = ewl_widget_name_find("main_win");
	entry = ewl_widget_name_find("pagenr_entry");
	dialog = ewl_widget_name_find("gotopage_dialog");

	if(!strcmp(e->base.keyname, "Escape")) {
		s = ewl_text_text_get(EWL_TEXT(entry));
		if(s && (strlen(s) > 0)) {
			free(s);
			ewl_entry_delete_left(EWL_ENTRY(entry));
		} else {
			ewl_widget_hide(dialog);
			ewl_widget_destroy(dialog);
			((GotoPageNumber *)data)->callback(-1);
			manual_update(true);
		}
	} else if(!strcmp(e->base.keyname, "Return")) {
		s = ewl_text_text_get(EWL_TEXT(entry));
		if(s) {
			n = atoi(s);
			free(s);
		}

		ewl_widget_hide(dialog);
		ewl_widget_destroy(dialog);
		manual_update(true);
		((GotoPageNumber *)data)->callback(n);
	}
	set_timer();
}

void ZLEwlGotoPageDialog(GotoPageNumber *gpn)
{
	Ewl_Widget *w, *label, *dialog, *entry;
	Ewl_Widget *entry_hbox;

	manual_update(false);

	w = ewl_widget_name_find("main_win");

	dialog = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_window_name_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_window_class_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_widget_name_set(dialog, "gotopage_dialog");
//	ewl_theme_data_str_set(EWL_WIDGET(dialog),"/window/group","ewl/dlg_entry");
	ewl_callback_append(dialog, EWL_CALLBACK_DELETE_WINDOW, ZLEwlGotoPageDialog_window_close_cb, NULL);
	ewl_callback_append(dialog, EWL_CALLBACK_KEY_UP, ZLEwlGotoPageDialog_key_up_cb, gpn);
	ewl_callback_append(dialog, EWL_CALLBACK_REVEAL, ZLEwlGotoPageDialog_reveal, NULL);
//	ewl_callback_append(dialog, EWL_CALLBACK_OBSCURE, ZLEwlGotoPageDialog_obscure, NULL);
	ewl_callback_append(dialog, EWL_CALLBACK_REALIZE, ZLEwlGotoPageDialog_realize, NULL);
	ewl_callback_append(dialog, EWL_CALLBACK_UNREALIZE, ZLEwlGotoPageDialog_unrealize, NULL);
	ewl_window_transient_for(EWL_WINDOW(dialog), EWL_WINDOW(w));
	ewl_window_dialog_set(EWL_WINDOW(dialog), 1);
	ewl_window_keyboard_grab_set(EWL_WINDOW(dialog), 1);
	EWL_EMBED(dialog)->x = 600;
	EWL_EMBED(dialog)->y = 0;
	ewl_widget_show(dialog);

	entry_hbox = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(dialog), entry_hbox);
	ewl_widget_show(entry_hbox);

	label = ewl_label_new();
	ewl_label_text_set(EWL_LABEL(label), "Page number:");
	ewl_container_child_append(EWL_CONTAINER(entry_hbox), label);
	ewl_widget_show(label);

	entry = ewl_entry_new();
	ewl_object_custom_w_set(EWL_OBJECT(entry), 70);
	ewl_container_child_append(EWL_CONTAINER(entry_hbox), entry);
//	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/file", get_theme_file());
//	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/group","ewl/dlg_entry");
//	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/cursor/group","ewl/dlg_entry/cursor");
//	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/selection_area/group","ewl/dlg_entry/selection");

	ewl_widget_name_set(entry, "pagenr_entry");
	ewl_widget_show(entry);

	ewl_widget_focus_send(dialog);
	ewl_widget_focus_send(entry);
}

// Options dialogs
Ewl_Widget *font_family_choicebox, *font_size_choicebox, *line_space_choicebox;
FBReader *myFbreader;
ZLPaintContext *myContext;

void redraw_text()
{
//	manual_update(true);
	myFbreader->clearTextCaches();
    myFbreader->refreshWindow();
//	manual_update(false);
}

void font_family_choicehandler(int choice, Ewl_Widget *parent)
{
	ZLStringOption &option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
	option.setValue(myContext->fontFamilies().at(choice));
    fini_choicebox(font_family_choicebox);

	redraw_text();
}

void font_size_choicehandler(int choice, Ewl_Widget *parent)
{
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	option.setValue(choice + 8);
    fini_choicebox(font_size_choicebox);

	redraw_text();
}

void line_space_choicehandler(int choice, Ewl_Widget *parent)
{
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	option.setValue(choice * 10 + 50);
	fini_choicebox(line_space_choicebox);

	redraw_text();
}

void options_dialog_choicehandler(int choice, Ewl_Widget *parent)
{
	if (choice == 0) {
		char **initchoices = (char **)malloc(myContext->fontFamilies().size() * sizeof(char*));
		for(int i = 0; i < myContext->fontFamilies().size(); i++) {
			asprintf(&initchoices[i], "%d. %s", (i % 8) + 1, myContext->fontFamilies().at(i).c_str());
		}

		ewl_widget_show(font_family_choicebox =
				init_choicebox((const char**)initchoices, myContext->fontFamilies().size(),
					font_family_choicehandler, parent));
	} else if (choice == 1) {
		int count = 11;
		char **initchoices = (char **)malloc(count * sizeof(char*));		
		for(int i = 0; i < count; i++) {		
			asprintf(&initchoices[i], "%d. %dpt", (i % 8) + 1, i + 8);
		}

		ewl_widget_show(font_size_choicebox =
				init_choicebox((const char**)initchoices, count,
					font_size_choicehandler, parent));
/*	} else if (choice == 2) {
		const char *initchoices[] =
		{ "1. Left Margin", "2. Right Margin", "3. Top Margin",
			"4. Bottom Margin" };
		ewl_widget_show(margin_choicebox =
				init_choicebox(initchoices, 4,
					margin_choicehandler, parent));
*/	} else if (choice == 2) {
		int count = 16;
		char **initchoices = (char **)malloc(count * sizeof(char*));		
		for(int i = 0; i < count; i++) {		
			asprintf(&initchoices[i], "%d. %d%%", (i % 8) + 1, i * 10 + 50);
		}
		ewl_widget_show(line_space_choicebox =
				init_choicebox((const char**)initchoices, count,
					line_space_choicehandler, parent));					
	}
}

void ZLEwlOptionsDialog(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;
	myContext = &(*f.context());

	const char *initchoices[] = { 
		"1. Font Family",
		"2. Font Size",
		//"3. Margins",
		"3. Line Spacing"
	};

	ewl_widget_show(init_choicebox(initchoices, 3, options_dialog_choicehandler, w, true));
}
