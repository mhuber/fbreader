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

#include <ZLLanguageList.h>
#include <ZLFile.h>

#include "ZLEwlDialogs.h"
#include "ZLEwlChoicebox.h"
#include "ZLEwlEntry.h"
#include "ZLEwlMessage.h"
#include "virtk.h"

#include <ewl/Ewl.h>

#include <iostream>
#include <cstdlib>
#include <cstring>

#include "../util/ZLEwlUtil.h"
#include "../../../../../fbreader/src/fbreader/FBReader.h"
#include "../../../../../fbreader/src/fbreader/FBReaderActions.h"
#include "../../../../../zlibrary/text/src/view/ZLTextStyle.h"
#include "../../../../../fbreader/src/fbreader/FBView.h"
#include "../../../../../fbreader/src/bookmodel/BookModel.h"
#include "../../../../../fbreader/src/fbreader/BookTextView.h"
#include "../../../../../fbreader/src/formats/FormatPlugin.h"
#include "../../../../../fbreader/src/description/BookDescriptionUtil.h"
#include "../../../../../fbreader/src/description/BookDescriptionUtil.h"
#include "../../../../../zlibrary/core/src/encoding/ZLEncodingConverter.h"
#include "../../../../../fbreader/src/formats/txt/TxtPlugin.h"
#include "../../../../../fbreader/src/formats/txt/PlainTextFormat.h"

#include "ZLEwlChoicebox_new.h"

vector<cb_olist *> olists;
cb_vlist *vlist;

bool turbo = false;

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
}

static void ZLEwlGotoPageDialog_key_up_cb(Ewl_Widget *w, void *ev, void *data)
{
	Ewl_Event_Key_Down *e;
	Ewl_Widget *entry, *dialog;
	char *s;
	int n;

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
		}
	} else if(!strcmp(e->base.keyname, "Return")) {
		s = ewl_text_text_get(EWL_TEXT(entry));
		if(s) {
			n = atoi(s);
			free(s);
		}

		ewl_widget_hide(dialog);
		ewl_widget_destroy(dialog);
		((GotoPageNumber *)data)->callback(n);
	}
}

void ZLEwlGotoPageDialog(GotoPageNumber *gpn)
{
	Ewl_Widget *w, *label, *dialog, *entry;
	Ewl_Widget *entry_hbox;


//	ewl_theme_theme_set("./oitheme.edj");

	w = ewl_widget_name_find("main_win");

	dialog = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_window_name_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_window_class_set(EWL_WINDOW(dialog), "Go to Page");
	ewl_widget_name_set(dialog, "gotopage_dialog");
	//ewl_theme_data_str_set(EWL_WIDGET(dialog),"/window/group","ewl/oi_window");
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
	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/group","ewl/dlg_entry");
	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/cursor/group","ewl/dlg_entry/cursor");
	ewl_theme_data_str_set(EWL_WIDGET(entry),"/entry/selection_area/group","ewl/dlg_entry/selection");

	ewl_widget_name_set(entry, "pagenr_entry");
	ewl_widget_show(entry);

	ewl_widget_focus_send(dialog);
	ewl_widget_focus_send(entry);
}

// Options dialogs
Ewl_Widget *font_family_choicebox, *font_size_choicebox, *line_space_choicebox, *margin_option,
		   *fl_indent_option;
FBReader *myFbreader;
ZLPaintContext *myContext;

void redraw_text()
{
	myFbreader->clearTextCaches();
    myFbreader->refreshWindow();
}

void font_family_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	ZLStringOption &option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
	option.setValue(myContext->fontFamilies().at(choice));

	update_label(choicebox_get_parent(parent), 0, myContext->fontFamilies().at(choice).c_str());

    fini_choicebox(font_family_choicebox);

	redraw_text();
}

void font_size_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	option.setValue(choice + 6);
	char *o;
	asprintf(&o, "%dpt", option.value());
	update_label(choicebox_get_parent(parent), 1, o);
	if(o)
		free(o);
    fini_choicebox(font_size_choicebox);

	redraw_text();
}

void line_space_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	option.setValue(choice * 10 + 50);
	char *o;
	asprintf(&o, "%d%%", option.value());
	update_label(choicebox_get_parent(parent), 3, o);
	if(o)
		free(o);
	fini_choicebox(line_space_choicebox);

	redraw_text();
}

void margin_handler(int nr, int value, Ewl_Widget *parent)
{
	char *o = NULL;
	if(value >= 0) {
		FBMargins &margins = FBView::margins();
		switch(nr) {
			case 0:
				margins.LeftMarginOption.setValue(value);
				asprintf(&o, "%d", margins.LeftMarginOption.value());
				break;
			case 1:
				margins.RightMarginOption.setValue(value);
				asprintf(&o, "%d", margins.RightMarginOption.value());
				break;
			case 2:
				margins.TopMarginOption.setValue(value);
				asprintf(&o, "%d", margins.TopMarginOption.value());
				break;
			case 3:
				margins.BottomMarginOption.setValue(value);
				asprintf(&o, "%d", margins.BottomMarginOption.value());
				break;
			default:
				return;
		}
		if(o) {
			update_label(entry_get_parent(parent), nr, o);
			free(o);
		}
	}

	fini_entry(margin_option);

	redraw_text();
}

void margin_handler_0(int value, Ewl_Widget *parent) { margin_handler(0, value, parent); }
void margin_handler_1(int value, Ewl_Widget *parent) { margin_handler(1, value, parent); }
void margin_handler_2(int value, Ewl_Widget *parent) { margin_handler(2, value, parent); }
void margin_handler_3(int value, Ewl_Widget *parent) { margin_handler(3, value, parent); }

void margins_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	std::string text;
	int value;
	void (*f)(int, Ewl_Widget*);
	FBMargins &margins = FBView::margins();

	switch(choice) {
		case 0:
			text = "Left Margin";
			f = margin_handler_0;
			value = margins.LeftMarginOption.value();
			break;
		case 1:
			text = "Right Margin";
			f = margin_handler_1;
			value = margins.RightMarginOption.value();
			break;
		case 2:
			text = "Top Margin";
			f = margin_handler_2;
			value = margins.TopMarginOption.value();
			break;
		case 3:
			text = "Bottom Margin";
			f = margin_handler_3;
			value = margins.BottomMarginOption.value();
			break;
		default:
			return;
	}

	ewl_widget_show(margin_option =
			init_entry((char *)text.c_str(), value, f, parent));
}

void alignment_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if(choice >= 0 && choice < 4) {
		ZLTextStyleCollection::instance().baseStyle().AlignmentOption.setValue(choice+1);

		char *alignments[] = { "undefined", "left", "right", "center", "justify" };
		update_label(choicebox_get_parent(parent), 4, alignments[choice+1]);

		fini_choicebox(parent);
		redraw_text();
	}
}

void fl_indent_handler(int value, Ewl_Widget *parent)
{
	if(value >= 0) {
		ZLTextStyleCollection &collection = ZLTextStyleCollection::instance();
		ZLTextFullStyleDecoration *decoration = (ZLTextFullStyleDecoration*)collection.decoration(/*REGULAR*/0);
		decoration->FirstLineIndentDeltaOption.setValue(value);

		char *o;
		asprintf(&o, "%d", value);
		update_label(entry_get_parent(parent), 6, o);
		if(o)
			free(o);
	}
	fini_entry(fl_indent_option);

	redraw_text();
}

void book_info_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	fini_choicebox(parent);
	ZLEwlBookInfo(*myFbreader);
}

void format_style_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if (choice == 0) {
		// 1. Font family
		char **initchoices = (char **)malloc(myContext->fontFamilies().size() * sizeof(char*));
		char **values = (char **)malloc(myContext->fontFamilies().size() * sizeof(char*));		
		for(int i = 0; i < myContext->fontFamilies().size(); i++) {
			asprintf(&initchoices[i], "%d. %s", (i % 8) + 1, myContext->fontFamilies().at(i).c_str());
			asprintf(&values[i], "");
		}

		ewl_widget_show(font_family_choicebox =
				init_choicebox((const char**)initchoices, (const char**)values, myContext->fontFamilies().size(),
					font_family_choicehandler, "Font Family", parent));
	} else if (choice == 1) {
		// 2. Font size
		int count = 13;
		char **initchoices = (char **)malloc(count * sizeof(char*));		
		char **values = (char **)malloc(count * sizeof(char*));		
		for(int i = 0; i < count; i++) {		
			asprintf(&initchoices[i], "%d. %dpt", (i % 8) + 1, i + 6);
			asprintf(&values[i], "");
		}

		ewl_widget_show(font_size_choicebox =
				init_choicebox((const char**)initchoices, (const char**)values, count,
					font_size_choicehandler, "Font Size", parent));
	} else if (choice == 2) {
		// 3. Bold
		ZLBooleanOption &fbold_option = ZLTextStyleCollection::instance().baseStyle().BoldOption;
		fbold_option.setValue(fbold_option.value() ? false : true);
		update_label(parent, 2, fbold_option.value() ? "On" : "Off");
		redraw_text();
	} else if (choice == 3) {
		// 4. Line Spacing
		int count = 16;
		char **initchoices = (char **)malloc(count * sizeof(char*));		
		char **values = (char **)malloc(count * sizeof(char*));		
		for(int i = 0; i < count; i++) {		
			asprintf(&initchoices[i], "%d. %d%%", (i % 8) + 1, i * 10 + 50);
			asprintf(&values[i], "");
		}
		ewl_widget_show(line_space_choicebox =
				init_choicebox((const char**)initchoices, (const char**)values, count,
					line_space_choicehandler, "Line Spacing", parent));
	} else if (choice == 4) {
		// 5. Alignment
		const char *initchoices[] = {
			"1. left",
			"2. right",
			"3. center",
			"4. justify",
		};

		const char *values[] = {"", "", "", ""};

		ewl_widget_show(init_choicebox(initchoices, values, 4, alignment_choicehandler, "Alignment", parent));
	} else if (choice == 5) {
		// 6. Margins
		const char *initchoices[] = { 
			"1. Left Margin",
			"2. Right Margin",
			"3. Top Margin",
			"4. Bottom Margin",
		};

		char *m0, *m1, *m2, *m3;
		FBMargins &margins = FBView::margins();
		asprintf(&m0, "%d", margins.LeftMarginOption.value());
		asprintf(&m1, "%d", margins.RightMarginOption.value());
		asprintf(&m2, "%d", margins.TopMarginOption.value());
		asprintf(&m3, "%d", margins.BottomMarginOption.value());
		const char *values[] = { 
			m0,
			m1,
			m2,
			m3,
		};

		ewl_widget_show(init_choicebox(initchoices, values, 4, margins_choicehandler, "Margins", parent));
	} else if (choice == 6) {
		// 7. First Line Indent
		ZLTextStyleCollection &collection = ZLTextStyleCollection::instance();
		ZLTextFullStyleDecoration *decoration = (ZLTextFullStyleDecoration*)collection.decoration(/*REGULAR*/0);

		ewl_widget_show(fl_indent_option =
				init_entry("First Line Indent", decoration->FirstLineIndentDeltaOption.value(), fl_indent_handler, parent));
	} else if (choice == 7) {
		// 8. Auto Hyphenations
		ZLBooleanOption &ah_option = ZLTextStyleCollection::instance().baseStyle().AutoHyphenationOption;
		ah_option.setValue(ah_option.value() ? false : true);
		update_label(parent, 7, ah_option.value() ? "On" : "Off");
		redraw_text();
	}
}

void format_style()
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myContext = &(*myFbreader->context());

	const char *initchoices[] = { 		
		"1. Font Family",
		"2. Font Size",
		"3. Bold",
		"4. Line Spacing",
		"5. Alignment",
		"6. Margins",
		"7. First Line Indent",
		"8. Auto Hyphenations",
	};

	ZLStringOption &ff_option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
	ZLIntegerRangeOption &fs_option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	ZLBooleanOption &fbold_option = ZLTextStyleCollection::instance().baseStyle().BoldOption;
	ZLIntegerOption &lsp_option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	ZLIntegerOption &al_option = ZLTextStyleCollection::instance().baseStyle().AlignmentOption;
	ZLBooleanOption &ah_option = ZLTextStyleCollection::instance().baseStyle().AutoHyphenationOption;
	ZLTextStyleCollection &collection = ZLTextStyleCollection::instance();
	ZLTextFullStyleDecoration *decoration = (ZLTextFullStyleDecoration*)collection.decoration(/*REGULAR*/0);
	char *fs_option_c, *lsp_option_c, *fl_option_c;
	asprintf(&fs_option_c, "%dpt", fs_option.value());
	asprintf(&lsp_option_c, "%d%%", lsp_option.value());
	asprintf(&fl_option_c, "%d", decoration->FirstLineIndentDeltaOption.value());

	char *alignments[] = { "undefined", "left", "right", "center", "justify" };

	const char *values[] = {
		ff_option.value().c_str(),
		fs_option_c,
		fbold_option.value() ? "On" : "Off",
		lsp_option_c,
		alignments[al_option.value()],
		"",
		fl_option_c,
		ah_option.value() ? "On" : "Off",
	};

	ewl_widget_show(init_choicebox(initchoices, values, 8, format_style_choicehandler, "Format & Style", w, true));

	//FIXME
	free(fs_option_c);
	free(fl_option_c);
	free(lsp_option_c);
}

void def_enc_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	const std::vector<ZLEncodingConverterInfoPtr> *pinfos = NULL;

	bool found = false;
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
		pinfos = &(*it)->infos();

		for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = pinfos->begin(); !found && (jt != pinfos->end()); ++jt)
			if ((*jt)->name() == PluginCollection::instance().DefaultEncodingOption.value())
				found = true;
	}

	if(choice >= pinfos->size())
		return;

	std::string newenc = (*pinfos).at(choice)->name();

	PluginCollection::instance().DefaultEncodingOption.setValue(newenc);

	update_label(choicebox_get_parent(parent), 3, newenc.c_str());

	fini_choicebox(parent);
}

void def_encset_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	if(choice > sets.size())
		return;

	update_label(choicebox_get_parent(parent), 2, sets.at(choice)->name().c_str());

	PluginCollection::instance().DefaultEncodingOption.setValue(sets.at(choice)->infos().at(0)->name());
	update_label(choicebox_get_parent(parent), 3, sets.at(choice)->infos().at(0)->name().c_str());

	fini_choicebox(parent);
}

void def_def_language_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	const std::vector<std::string> &l = ZLLanguageList::languageCodes();

	if(choice <= l.size()) {
		std::string lang;
		if(choice == l.size())
			lang = "other";
		else
			lang = l.at(choice);

		PluginCollection::instance().DefaultLanguageOption.setValue(lang);

		update_label(choicebox_get_parent(parent), 1, ZLLanguageList::languageName(PluginCollection::instance().DefaultLanguageOption.value()).c_str());

		fini_choicebox(parent);
	}
}

void def_language_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if(choice == 0) {
		PluginCollection::instance().LanguageAutoDetectOption.setValue(
				PluginCollection::instance().LanguageAutoDetectOption.value() ?
				false : true);
		update_label(parent, 0, PluginCollection::instance().LanguageAutoDetectOption.value() ? "On" : "Off");
	} else if(choice == 1) {
		const std::vector<std::string> &l = ZLLanguageList::languageCodes();

		char **initchoices = (char **)malloc((l.size() + 1) * sizeof(char*));
		char **values = (char **)malloc((l.size() + 1) * sizeof(char*));
		for(unsigned int i = 0; i < l.size(); i++) {
			asprintf(&initchoices[i], "%d. %s", i % 8 + 1, ZLLanguageList::languageName(l.at(i)).c_str());
			asprintf(&values[i], "");
		}
		int i = l.size();
		asprintf(&initchoices[i], "%d. %s", i % 8 + 1, ZLLanguageList::languageName("other").c_str());
		asprintf(&values[i], "");

		ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, 1 + l.size(), def_def_language_choicehandler, "Default Language", parent, false));
	} else if(choice == 2) {
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			char **initchoices = (char **)malloc(sets.size() * sizeof(char*));
			char **values = (char **)malloc(sets.size() * sizeof(char*));
			for(unsigned int i = 0; i < sets.size(); i++) {
				asprintf(&initchoices[i], "%d. %s", i % 8 + 1, sets.at(i)->name().c_str());
				asprintf(&values[i], "");
			}
			ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, sets.size(), def_encset_choicehandler, "Default Encoding Set", parent, false));
	} else if(choice == 3) {
			const std::vector<ZLEncodingConverterInfoPtr> *pinfos = NULL;

			bool found = false;
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
				pinfos = &(*it)->infos();

				for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = pinfos->begin(); !found && (jt != pinfos->end()); ++jt)
					if ((*jt)->name() == PluginCollection::instance().DefaultEncodingOption.value())
						found = true;
			}

			const std::vector<ZLEncodingConverterInfoPtr> &infos = *pinfos;

			char **initchoices = (char **)malloc(infos.size() * sizeof(char*));
			char **values = (char **)malloc(infos.size() * sizeof(char*));
			for(unsigned int i = 0; i < infos.size(); i++) {
				asprintf(&initchoices[i], "%d. %s", i % 8 + 1, infos.at(i)->visibleName().c_str());
				asprintf(&values[i], "");
			}
			ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, infos.size(), def_enc_choicehandler, "Default Encoding", parent, false));
	} else if(choice == 4) {
		ZLEncodingCollection::useWindows1252HackOption().setValue(ZLEncodingCollection::useWindows1252HackOption().value() ? false : true);
		update_label(parent, 4, ZLEncodingCollection::useWindows1252HackOption().value() ? "On" : "Off");
	}
}

void language()
{
	int cnt = 5;
	char **initchoices = (char **)malloc(cnt * sizeof(char*));
	char **values = (char **)malloc(cnt * sizeof(char*));

	int i = 0;
	asprintf(&initchoices[i], "%d. Detect Language and Encoding", i % 8 + 1);
	asprintf(&values[i++], "%s", PluginCollection::instance().LanguageAutoDetectOption.value() ? "On" : "Off");

	asprintf(&initchoices[i], "%d. Default Language", i % 8 + 1);
	if(ZLLanguageList::languageName(PluginCollection::instance().DefaultLanguageOption.value()) == "????????")
		asprintf(&values[i++], "%s", ZLLanguageList::languageName("other").c_str());
	else
		asprintf(&values[i++], "%s", ZLLanguageList::languageName(PluginCollection::instance().DefaultLanguageOption.value()).c_str());
	asprintf(&initchoices[i], "%d. Default Encoding Set", i % 8 + 1);

	bool found = false;
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
		const std::vector<ZLEncodingConverterInfoPtr> &infos = (*it)->infos();

		for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = infos.begin(); !found && (jt != infos.end()); ++jt) {
			if ((*jt)->name() == PluginCollection::instance().DefaultEncodingOption.value()) {
				asprintf(&values[i++], (*it)->name().c_str());

				asprintf(&initchoices[i], "%d. Default Encoding", i % 8 + 1);
				asprintf(&values[i++], (*jt)->name().c_str());
				//(*jt)->visibleName().c_str());
				//myBookInfo->EncodingOption.value().c_str());

				found =true;
				break;
			}
		}
	}
	asprintf(&initchoices[i], "%d: iso-8859-1 -> win-1251", i % 8 + 1);
	asprintf(&values[i], "%s", ZLEncodingCollection::useWindows1252HackOption().value() ? "On" : "Off");

	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, cnt, def_language_choicehandler, "Language", ewl_widget_name_find("main_win"), true));
}

Ewl_Widget *ind_h, *ind_o, *ind_f;

void ind_height_handler(int value, Ewl_Widget *parent)
{
	if(value > 0) {
		FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();
		indicatorInfo.HeightOption.setValue(value);
		char *l;
		asprintf(&l, "%d", value);
		update_label(entry_get_parent(parent), 4, l);
		free(l);
	}
	fini_entry(parent);
	redraw_text();
}

void ind_offset_handler(int value, Ewl_Widget *parent)
{
	if(value < 0)
		value = 0;

	FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();
	indicatorInfo.OffsetOption.setValue(value);
	char *l;
	asprintf(&l, "%d", value);
	update_label(entry_get_parent(parent), 5, l);
	free(l);
	fini_entry(parent);
	redraw_text();
}

void ind_fontsize_handler(int value, Ewl_Widget *parent)
{
	if(value < 4)
		value = 4;
	FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();
	indicatorInfo.FontSizeOption.setValue(value);
	char *l;
	asprintf(&l, "%dpt", value);
	update_label(entry_get_parent(parent), 6, l);
	free(l);
	fini_entry(parent);
	redraw_text();
}

#define toggle_bool_option(a, b) a.setValue(a.value() ? false : true); update_label(parent, b, a.value() ? "On" : "Off");

void indicator_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();
	switch(choice) {
		case 0:
			indicatorInfo.TypeOption.setValue(
					(indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR) ?
					ZLTextPositionIndicatorInfo::NONE : ZLTextPositionIndicatorInfo::FB_INDICATOR);

			update_label(parent, 0, (indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR) ? "On" : "Off");
	redraw_text();
			break;
		case 1:
			toggle_bool_option(myFbreader->bookTextView().ShowTOCMarksOption, 1);
	redraw_text();
			break;
		case 2:
			toggle_bool_option(indicatorInfo.ShowTextPositionOption, 2);
	redraw_text();
			break;
		case 3:
			toggle_bool_option(indicatorInfo.ShowTimeOption, 3);
	redraw_text();
			break;
		case 4:
			ewl_widget_show(ind_h = init_entry("Indicator Height", indicatorInfo.HeightOption.value(), ind_height_handler, parent));
			break;
		case 5:
			ewl_widget_show(ind_o = init_entry("Indicator Offset", indicatorInfo.OffsetOption.value(), ind_offset_handler, parent));
			break;
		case 6:
			ewl_widget_show(ind_f = init_entry("Indicator Font Size", indicatorInfo.FontSizeOption.value(), ind_fontsize_handler, parent));
			break;
	}
}

void indicator()
{
	FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();

	int cnt = 7;
	char **initchoices = (char **)malloc(cnt * sizeof(char*));
	char **values = (char **)malloc(cnt * sizeof(char*));

	int i = 0;
	asprintf(&initchoices[i], "%d. Show Indicator", i % 8 + 1);
	asprintf(&values[i++], "%s", (indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR) ? "On" : "Off");

	asprintf(&initchoices[i], "%d. Show TOC Marks", i % 8 + 1);
	asprintf(&values[i++], "%s", myFbreader->bookTextView().ShowTOCMarksOption.value() ? "On" : "Off");

	asprintf(&initchoices[i], "%d. Show Position", i % 8 + 1);
	asprintf(&values[i++], "%s", indicatorInfo.ShowTextPositionOption.value() ? "On" : "Off");

	asprintf(&initchoices[i], "%d. Show Time and Battery", i % 8 + 1);
	asprintf(&values[i++], "%s", indicatorInfo.ShowTimeOption.value() ? "On" : "Off");

	asprintf(&initchoices[i], "%d. Indicator Height", i % 8 + 1);
	asprintf(&values[i++], "%d", indicatorInfo.HeightOption.value());

	asprintf(&initchoices[i], "%d. Offset from Text", i % 8 + 1);
	asprintf(&values[i++], "%d", indicatorInfo.OffsetOption.value());

	asprintf(&initchoices[i], "%d. Font Size", i % 8 + 1);
	asprintf(&values[i++], "%dpt", indicatorInfo.FontSizeOption.value());

	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, cnt, indicator_choicehandler, "Indicator", ewl_widget_name_find("main_win"), true));
}

void options_dialog_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if(choice == 0) {
		// Book Info
		//fini_choicebox(parent);
		ZLEwlBookInfo(*myFbreader);
	} else if(choice == 1) {
		// Language
		//fini_choicebox(parent);
		language();
	} else if(choice == 2) {
		// Format & Style
		//fini_choicebox(parent);
		format_style();
	} else if(choice == 3) {
		// Indicator
		//fini_choicebox(parent);
		indicator();
	} else if(choice == 4) {
		// Turbo Mode
		if(turbo) {
			turbo = false;
			update_label(parent, 4, "Off");
		} else {
			turbo = true;
			update_label(parent, 4, "On");;
		}
	}
}

/*void ZLEwlOptionsDialog(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;
	myContext = &(*f.context());

	const char *initchoices[] = {
		"1. Book Info",
		"2. Language",
		"3. Format & Style",
		"4. Indicator",
		"5. Turbo mode",
	};

	const char *values[] = {
		"", "", "", "",
		turbo ? "On" : "Off",
	};

	ewl_widget_show(init_choicebox(initchoices, values, 5, options_dialog_choicehandler, "Settings", w, true));
}
*/


ZLTextTreeParagraph *curTOCParent;

void toc_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	ZLTextTreeParagraph *selEntry;
	std::vector<ZLTextTreeParagraph*> list;
	ContentsModel &cm = (ContentsModel&)*myFbreader->myModel->contentsModel();

	if(curTOCParent != cm.myRoot) {
		if(choice == 0)
			selEntry = curTOCParent->parent();
		else if(choice == 1)
			selEntry = curTOCParent;
		else
			selEntry = curTOCParent->children().at(choice - 2);
	} else {
		selEntry = curTOCParent->children().at(choice);
	}

	list = selEntry->children();
	if(list.empty() || (curTOCParent == selEntry)) {
		fini_choicebox(parent, false);
		myFbreader->bookTextView().gotoParagraph(cm.reference(selEntry));
		myFbreader->refreshWindow();
		return;
	}

	curTOCParent = selEntry;

	char **initchoices = (char **)malloc((list.size() + 2) * sizeof(char*));
	char **values = (char **)malloc((list.size() + 2) * sizeof(char*));

	int cnt = 0;
	if(selEntry != cm.myRoot) {
		asprintf(&initchoices[cnt], "1. ..");
		asprintf(&values[cnt], "");
		cnt++;
		asprintf(&initchoices[cnt], "%d. .", cnt + 1);
		asprintf(&values[cnt], "");
		cnt++;
	}

	short len;
	char *p;
	for(int i = 0; i < list.size(); i++) {
		p = list.at(i)->myFirstEntryAddress;

		len = 0;
		memcpy(&len, p + 3, sizeof(short));
		char *t = (char*)malloc((len + 1) * sizeof(char));
		bzero(t, len + 1);
		memcpy(t, p + 7, len);

		if(list.at(i)->children().empty())
			asprintf(&initchoices[cnt], "%d. %s", cnt % 8 + 1, t);
		else
			asprintf(&initchoices[cnt], "%d. + %s", cnt % 8 + 1, t);
		free(t);

		asprintf(&values[cnt], "");
		cnt++;
	}

	update_choicebox(parent, (const char **)initchoices, (const char **)values, cnt, true); 

//	fini_choicebox(parent, false);
//	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, cnt, toc_choicehandler, "TOC", w, true));
}

void ZLEwlTOCDialog(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;

	int cnt = 0;
	ContentsModel &cm = (ContentsModel&)*myFbreader->myModel->contentsModel();
	for(int i = 0; i < cm.paragraphsNumber(); i++)
		if(((ZLTextTreeParagraph*)cm[i])->parent() == cm.myRoot)
			cnt++;

	char **initchoices = (char **)malloc(cnt * sizeof(char*));
	char **values = (char **)malloc(cnt * sizeof(char*));


	cnt = 0;
	short len;
	char *p;
	for(int i = 0; i < cm.paragraphsNumber(); i++) {
		if(((ZLTextTreeParagraph*)cm[i])->parent() == cm.myRoot) {
			p = ((ZLTextParagraph*)cm[i])->myFirstEntryAddress;

			len = 0;
			memcpy(&len, p + 3, sizeof(short));
			char *t = (char*)malloc((len + 1) * sizeof(char));
			bzero(t, len + 1);
			memcpy(t, p + 7, len);

			std::vector<ZLTextTreeParagraph*> vpar = ((ZLTextTreeParagraph*)cm[i])->children();

			if(vpar.empty())
				asprintf(&initchoices[cnt], "%d. %s", cnt % 8 + 1, t);
			else
				asprintf(&initchoices[cnt], "%d. + %s", cnt % 8 + 1, t);
			free(t);

			asprintf(&values[cnt], "");
			cnt++;
		}
	}


	curTOCParent = cm.myRoot;
	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, cnt, toc_choicehandler, "TOC", w, true));
}

// Bookmarks
void bmk_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if(lp) {
		myFbreader->bookTextView().removeBookmark(choice);

		std::vector<std::pair<std::pair<int, int>, std::pair<int, std::string> > > bookmarks
			= myFbreader->bookTextView().getBookmarks();

		char **initchoices = (char **)malloc(bookmarks.size() * sizeof(char*));
		char **values = (char **)malloc(bookmarks.size() * sizeof(char*));

		for(int i = 0; i < bookmarks.size(); i++) {
			asprintf(&initchoices[i], "%d. Page %d: %s", i % 8 + 1, bookmarks.at(i).second.first, bookmarks.at(i).second.second.c_str());
			asprintf(&values[i], "");
		}

		update_choicebox(parent, (const char **)initchoices, (const char **)values, bookmarks.size()); 
	} else {
		myFbreader->bookTextView().gotoBookmark(choice);
		//myFbreader->refreshWindow();

		fini_choicebox(parent);
	}
}

void ZLEwlBMKDialog(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;

	std::vector<std::pair<std::pair<int, int>, std::pair<int, std::string> > > bookmarks
		= myFbreader->bookTextView().getBookmarks();

	char **initchoices = (char **)malloc(bookmarks.size() * sizeof(char*));
	char **values = (char **)malloc(bookmarks.size() * sizeof(char*));

	for(int i = 0; i < bookmarks.size(); i++) {
			asprintf(&initchoices[i], "%d. Page %d: %s", i % 8 + 1, bookmarks.at(i).second.first, bookmarks.at(i).second.second.c_str());
			asprintf(&values[i], "");
	}

	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, bookmarks.size(), bmk_choicehandler, "Bookmarks", w, true));
}

void ZLEwlBMKAddedMsg(FBReader &f) {
	myFbreader = &f;
	ewl_widget_show(init_message("Bookmark added", true));
}

// search dialogs

static void search_info_reveal_cb(Ewl_Widget *w, void *ev, void *data) {
	ewl_window_move(EWL_WINDOW(w), (600 - CURRENT_W(w)) / 2, (800 - CURRENT_H(w)) - 200);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
}

static void search_info_realize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 0);
}

static void search_info_unrealize_cb(Ewl_Widget *w, void *ev, void *data) {
	Ewl_Widget *win;
	win = ewl_widget_name_find("main_win");
	if(win)
		ewl_window_keyboard_grab_set(EWL_WINDOW(win), 1);
}

static void search_info_keyhandler(Ewl_Widget *w, void *ev, void *data)
{
	Ewl_Event_Key_Down *e;
	Ewl_Widget *message, *dialog;
	char *s;

	e = (Ewl_Event_Key_Down*)ev;


	if(!strcmp(e->base.keyname, "Escape") || !strcmp(e->base.keyname, "Return")) {
		ewl_widget_destroy(w);
		myFbreader->myModel->bookTextModel()->removeAllMarks();
		redraw_text();
		return;
	} else if(!strcmp(e->base.keyname, "Up") || !strcmp(e->base.keyname, "0")) {
		if(myFbreader->bookTextView().canFindNext())
			myFbreader->bookTextView().findNext();
	} else if(!strcmp(e->base.keyname, "Down") || !strcmp(e->base.keyname, "9")) {
		if(myFbreader->bookTextView().canFindPrevious())
			myFbreader->bookTextView().findPrevious();
	}

	Ewl_Widget *l = ewl_widget_name_find("search_info_label");
	bool n = false, p = false;
	if(myFbreader->bookTextView().canFindNext())
		n = true;
	if(myFbreader->bookTextView().canFindPrevious())
		p = true;

	if(n && p) 
		ewl_label_text_set(EWL_LABEL(l), "Search <- ->");
	else if(n)
		ewl_label_text_set(EWL_LABEL(l), "Search    ->");
	else if(p)
		ewl_label_text_set(EWL_LABEL(l), "Search <-   ");
	else
		ewl_label_text_set(EWL_LABEL(l), "Text not found");
}

Ewl_Widget *init_search_info()
{
	Ewl_Widget *w, *label, *message, *message_hbox;

	w = ewl_window_new();
	ewl_window_title_set(EWL_WINDOW(w), "Message");
	ewl_window_name_set(EWL_WINDOW(w), "EWL_WINDOW");
	ewl_window_class_set(EWL_WINDOW(w), "Message");
	ewl_widget_name_set(w, "message_win");
	ewl_callback_append(w, EWL_CALLBACK_KEY_UP, search_info_keyhandler, NULL);
	ewl_callback_append(w, EWL_CALLBACK_REVEAL, search_info_reveal_cb, NULL);
	ewl_callback_append(w, EWL_CALLBACK_REALIZE, search_info_realize_cb, NULL);
	ewl_callback_append(w, EWL_CALLBACK_UNREALIZE, search_info_unrealize_cb, NULL);
	ewl_window_keyboard_grab_set(EWL_WINDOW(w), 1);
	EWL_EMBED(w)->x = 600;
	EWL_EMBED(w)->y = 0;
	ewl_widget_show(w);

	message_hbox = ewl_hbox_new();
	ewl_container_child_append(EWL_CONTAINER(w), message_hbox);
	ewl_widget_show(message_hbox);

	label = ewl_label_new();
	//ewl_label_text_set(EWL_LABEL(label), "Search next(0)/previous(9). Exit - OK");
	bool n = false, p = false;
	if(myFbreader->bookTextView().canFindNext())
		n = true;
	if(myFbreader->bookTextView().canFindPrevious())
		p = true;

	if(n && p) 
		ewl_label_text_set(EWL_LABEL(label), "Search <- ->");
	else if(n)
		ewl_label_text_set(EWL_LABEL(label), "Search    ->");
	else if(p)
		ewl_label_text_set(EWL_LABEL(label), "Search <-   ");
	else
		ewl_label_text_set(EWL_LABEL(label), "Text not found");
	ewl_widget_name_set(label, "search_info_label");
	ewl_container_child_append(EWL_CONTAINER(message_hbox), label);
	ewl_widget_show(label);

	ewl_widget_focus_send(w);

	return w;
}

void search_input_handler(char *text)
{
	if(text && strlen(text)) {
		if(myFbreader->bookTextView().search(std::string(text), true, false, false, false)) {
			ewl_widget_show(init_search_info());
		} else {
			redraw_text();
			ewl_widget_show(init_message("Text not found", true));;
		}
	} else {
		redraw_text();
	}
}

void ZLEwlSearchDialog(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;

	ewl_widget_show(init_virtk(w, "Search", search_input_handler));
}


// book info dialogs
BookInfo *myBookInfo;
BookDescriptionPtr description;

void set_encoding(std::string enc)
{
	BookDescriptionPtr description = new BookDescription(myFbreader->myModel->fileName());
	description->myEncoding = enc;
	BookDescriptionUtil::saveInfo(ZLFile(myFbreader->myModel->fileName()));

	myBookInfo->EncodingOption.setValue(enc);

	myFbreader->openFile(myFbreader->myModel->fileName());

	myFbreader->clearTextCaches();
    myFbreader->refreshWindow();
}

void enc_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	const std::vector<ZLEncodingConverterInfoPtr> *pinfos = NULL;

	bool found = false;
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
		pinfos = &(*it)->infos();

		for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = pinfos->begin(); !found && (jt != pinfos->end()); ++jt)
			if ((*jt)->name() == myBookInfo->EncodingOption.value())
				found = true;
	}

	if(choice >= pinfos->size())
		return;

	std::string newenc = (*pinfos).at(choice)->name();

	fini_choicebox(choicebox_get_parent(parent));
	fini_choicebox(parent);
	set_encoding(newenc);
	ZLEwlBookInfo(*myFbreader);
}

void encset_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	if(choice > sets.size())
		return;

	std::string newenc = sets.at(choice)->infos().at(0)->name();
	fini_choicebox(choicebox_get_parent(parent));
	fini_choicebox(parent);
	set_encoding(newenc);
	ZLEwlBookInfo(*myFbreader);
}

void language_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	BookDescriptionPtr description = new BookDescription(myFbreader->myModel->fileName());
	if(choice < ZLLanguageList::languageCodes().size())
		description->myLanguage = ZLLanguageList::languageCodes().at(choice);
	else
		description->myLanguage = "other";

	BookDescriptionUtil::saveInfo(ZLFile(myFbreader->myModel->fileName()));

	//update_label(choicebox_get_parent(parent), 0, description->myLanguage);
	//fini_choicebox(choicebox_get_parent(parent));

	// set encoding
	myBookInfo->LanguageOption.setValue(description->myLanguage);

	fini_choicebox(choicebox_get_parent(parent));
	fini_choicebox(parent);

	myFbreader->openFile(myFbreader->myModel->fileName());

	myFbreader->clearTextCaches();
    myFbreader->refreshWindow();

	ZLEwlBookInfo(*myFbreader);
}

char *break_type_values[] = { "New Line", "Empty Line", "Line With Indent" };
int curBreakType = 0;

void bookinfo_choicehandler(int choice, Ewl_Widget *parent, bool lp)
{
	if(choice == 0) {
		const std::vector<std::string> &l = ZLLanguageList::languageCodes();

		char **initchoices = (char **)malloc((l.size() + 1) * sizeof(char*));
		char **values = (char **)malloc((l.size() + 1) * sizeof(char*));
		for(unsigned int i = 0; i < l.size(); i++) {
			asprintf(&initchoices[i], "%d. %s", i % 8 + 1, ZLLanguageList::languageName(l.at(i)).c_str());
			asprintf(&values[i], "");
		}
		int i = l.size();
		asprintf(&initchoices[i], "%d. %s", i % 8 + 1, ZLLanguageList::languageName("other").c_str());
		asprintf(&values[i], "");

//		fini_choicebox(parent);
		ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, 1 + l.size(), language_choicehandler, "Language", parent, false));
	}

	if((myBookInfo->EncodingOption.value() != "auto") && ((choice == 1) || (choice == 2))) {
		if(choice == 1) {
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			char **initchoices = (char **)malloc(sets.size() * sizeof(char*));
			char **values = (char **)malloc(sets.size() * sizeof(char*));
			for(unsigned int i = 0; i < sets.size(); i++) {
				asprintf(&initchoices[i], "%d. %s", i % 8 + 1, sets.at(i)->name().c_str());
				asprintf(&values[i], "");
			}
//			fini_choicebox(parent);
			ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, sets.size(), encset_choicehandler, "Encoding Set", parent, false));
		} else if(choice == 2) {
			const std::vector<ZLEncodingConverterInfoPtr> *pinfos = NULL;

			bool found = false;
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
				pinfos = &(*it)->infos();

				for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = pinfos->begin(); !found && (jt != pinfos->end()); ++jt)
					if ((*jt)->name() == myBookInfo->EncodingOption.value())
						found = true;
			}

			const std::vector<ZLEncodingConverterInfoPtr> &infos = *pinfos;

			char **initchoices = (char **)malloc(infos.size() * sizeof(char*));
			char **values = (char **)malloc(infos.size() * sizeof(char*));
			for(unsigned int i = 0; i < infos.size(); i++) {
				asprintf(&initchoices[i], "%d. %s", i % 8 + 1, infos.at(i)->visibleName().c_str());
				asprintf(&values[i], "");
			}
//			fini_choicebox(parent);
			ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, infos.size(), enc_choicehandler, "Encoding", parent, false));
		}
	}

	if(((myBookInfo->EncodingOption.value() != "auto") && (choice == 3)) ||
		((myBookInfo->EncodingOption.value() == "auto") && (choice == 1))) {

		FormatPlugin *plugin = PluginCollection::instance().plugin(ZLFile(myFbreader->myModel->fileName()), false);
		if (plugin != 0) {
			TxtPlugin *test = dynamic_cast<TxtPlugin*>(plugin);
			if(test != NULL) {
				PlainTextFormat myFormat(myFbreader->myModel->fileName());
				if (!myFormat.initialized()) {
					PlainTextFormatDetector detector;
					shared_ptr<ZLInputStream> stream = ZLFile(myFbreader->myModel->fileName()).inputStream();
					if (!stream.isNull()) {
						detector.detect(*stream, myFormat);
					}
				}

				curBreakType << 1;
				curBreakType %= 5;

				myFormat.BreakTypeOption.setValue(curBreakType);

				fini_choicebox(parent);

				myFbreader->openFile(myFbreader->myModel->fileName());

				myFbreader->clearTextCaches();
				myFbreader->refreshWindow();

				ZLEwlBookInfo(*myFbreader);
			}
		}
	}
}

void ZLEwlBookInfo(FBReader &f)
{
	Ewl_Widget *w = ewl_widget_name_find("main_win");

	myFbreader = &f;

	const std::string &fileName = f.myModel->fileName();
	myBookInfo = new BookInfo(fileName);
//	description = new BookDescription(fileName);


	int cnt = 20;
	char **initchoices = (char **)malloc(cnt * sizeof(char*));
	char **values = (char **)malloc(cnt * sizeof(char*));

	//FIXME
	int choice = 0;
	int i = 0;
	asprintf(&initchoices[i], "File: %s", ZLFile::fileNameToUtf8(ZLFile(fileName).name(false)).c_str());
	asprintf(&values[i++], "");
	asprintf(&initchoices[i], "Full path: %s", ZLFile::fileNameToUtf8(ZLFile(fileName).path()).c_str());
	asprintf(&values[i++], "");
	asprintf(&initchoices[i], "Title: %s", myBookInfo->TitleOption.value().c_str());
	asprintf(&values[i++], "");
	asprintf(&initchoices[i], "Author: %s", myBookInfo->AuthorDisplayNameOption.value().c_str());
	asprintf(&values[i++], "");

	if(!myBookInfo->SeriesNameOption.value().empty()) {
		asprintf(&initchoices[i], "Series: %s", myBookInfo->SeriesNameOption.value().c_str());
		asprintf(&values[i++], "");
		asprintf(&initchoices[i], "Book number: %d", myBookInfo->NumberInSeriesOption.value());
		asprintf(&values[i++], "");
	}

	asprintf(&initchoices[i], "%d. Language", ++choice);
	asprintf(&values[i++], ZLLanguageList::languageName(myBookInfo->LanguageOption.value()).c_str());
	if(myBookInfo->EncodingOption.value() == "auto") {
		asprintf(&initchoices[i], "Encoding");
		asprintf(&values[i++], myBookInfo->EncodingOption.value().c_str());
	} else {
		asprintf(&initchoices[i], "%d. Encoding Set", ++choice);

		bool found = false;
		const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
		for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
			const std::vector<ZLEncodingConverterInfoPtr> &infos = (*it)->infos();

			for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = infos.begin(); !found && (jt != infos.end()); ++jt) {
				if ((*jt)->name() == myBookInfo->EncodingOption.value()) {
					asprintf(&values[i++], (*it)->name().c_str());

					asprintf(&initchoices[i], "%d. Encoding", ++choice);
					asprintf(&values[i++], (*jt)->name().c_str());
							//(*jt)->visibleName().c_str());
							//myBookInfo->EncodingOption.value().c_str());

					found =true;
					break;
				}
			}
		}

	}

	FormatPlugin *plugin = PluginCollection::instance().plugin(ZLFile(fileName), false);
	if (plugin != 0) {
		TxtPlugin *test = dynamic_cast<TxtPlugin*>(plugin);
		if(test != NULL) {
			 PlainTextFormat myFormat(fileName);
			 if (!myFormat.initialized()) {
				 PlainTextFormatDetector detector;
				 shared_ptr<ZLInputStream> stream = ZLFile(fileName).inputStream();
				 if (!stream.isNull()) {
					 detector.detect(*stream, myFormat);
				 }
			 }

			 switch (myFormat.BreakTypeOption.value()) {
				 case PlainTextFormat::BREAK_PARAGRAPH_AT_NEW_LINE:
					 curBreakType = 0;
					 break;
				 case PlainTextFormat::BREAK_PARAGRAPH_AT_EMPTY_LINE:
					 curBreakType = 1;
					 break;
				 case PlainTextFormat::BREAK_PARAGRAPH_AT_EMPTY_LINE | PlainTextFormat::BREAK_PARAGRAPH_AT_LINE_WITH_INDENT:
				 default:
					 curBreakType = 2;
			 }

			 curBreakType;

			 asprintf(&initchoices[i], "%d. Break Paragraph at", ++choice);
			 asprintf(&values[i++], break_type_values[curBreakType]);
		}
	}
	//fixme
	cnt = i;

/*	while(i < cnt) {
		asprintf(&initchoices[i], "File %s", fileName.c_str());
		asprintf(&values[i++], "");
	}
*/


	ewl_widget_show(init_choicebox((const char **)initchoices, (const char **)values, cnt, bookinfo_choicehandler, "Book Information", w, true));
}

void margins_handler(int idx)
{
}

void format_style_handler(int idx)
{
	cb_olist *current_olist = olists.back();
	if(2 == idx) {
		cb_olist_item *i = &current_olist->items.at(idx);
		++i->curval_idx %= i->values.size();

		ZLTextStyleCollection::instance().baseStyle().BoldOption.setValue(i->values.at(i->curval_idx).ibool);

		cb_lcb_invalidate(idx);
	} else if(5 == idx) {
		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Margins";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = margins_handler;

		cb_olist_item i;

		FBMargins &margins = FBView::margins();

		ADD_OPTION_INT("Left Margin", margins.LeftMarginOption.value());
		ADD_OPTION_INT("Right Margin", margins.RightMarginOption.value());
		ADD_OPTION_INT("Top Margin", margins.TopMarginOption.value());
		ADD_OPTION_INT("Bottom Margin", margins.BottomMarginOption.value());

		cb_lcb_redraw();
	}
}

void indicator_handler(int idx)
{
}

void language_handler(int idx)
{
}

void options_dialog_handler(int idx)
{
	fprintf(stderr, "options_dialog_handler: %d\n", idx);

	cb_olist *current_olist = olists.back();

	if(0 == idx) {
		myContext = &(*myFbreader->context());

		ZLTextBaseStyle &bs = ZLTextStyleCollection::instance().baseStyle();

		ZLTextStyleCollection &collection = ZLTextStyleCollection::instance();
		ZLTextFullStyleDecoration *decoration = (ZLTextFullStyleDecoration*)collection.decoration(/*REGULAR*/0);

		char *alignments[] = { "undefined", "left", "right", "center", "justify" };

		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Format & Style";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = format_style_handler;

		cb_olist_item i;

		ADD_OPTION_STRING("Font Family", bs.FontFamilyOption.value());
		ADD_OPTION_INT_F("Font Size", bs.FontSizeOption.value(), "%dpt");
		ADD_OPTION_BOOL("Bold", bs.BoldOption.value());
		ADD_OPTION_INT("Line Spacing", bs.LineSpacePercentOption.value());
		ADD_OPTION_INT_T("Alignment", bs.AlignmentOption.value(), alignments[bs.AlignmentOption.value()]);
		ADD_SUBMENU_ITEM("Margins");
		ADD_OPTION_INT("First Line Indent", decoration->FirstLineIndentDeltaOption.value());
		ADD_OPTION_BOOL("Auto Hyphenations", bs.AutoHyphenationOption.value());

		cb_lcb_redraw();
	} else if(1 == idx) {
		FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();

		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Indicator";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = indicator_handler;

		cb_olist_item i;

		ADD_OPTION_BOOL("Show Indicator", (indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR));
		ADD_OPTION_BOOL("Show TOC Marks", myFbreader->bookTextView().ShowTOCMarksOption.value());
		ADD_OPTION_BOOL("Show Position", indicatorInfo.ShowTextPositionOption.value());
		ADD_OPTION_BOOL("Show Time", indicatorInfo.ShowTimeOption.value());
		ADD_OPTION_BOOL("Show Battery", indicatorInfo.ShowBatteryOption.value());
		ADD_OPTION_INT("Indicator Height", indicatorInfo.HeightOption.value());
		ADD_OPTION_INT("Offset from Text", indicatorInfo.OffsetOption.value());
		ADD_OPTION_INT_F("Font Size", indicatorInfo.FontSizeOption.value(), "%dpt");

		cb_lcb_redraw();
	} else if(2 == idx) {
		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Language";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = language_handler;

		PluginCollection &pc = PluginCollection::instance();


		cb_olist_item i;

		ADD_OPTION_BOOL("Detect Language and Encoding", pc.LanguageAutoDetectOption.value());
		ADD_OPTION_STRING("Default Language",
				(ZLLanguageList::languageName(pc.DefaultLanguageOption.value()) == "????????") ?
				ZLLanguageList::languageName("other").c_str() :
				ZLLanguageList::languageName(pc.DefaultLanguageOption.value()).c_str());


		bool found = false;
		const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
		for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
			const std::vector<ZLEncodingConverterInfoPtr> &infos = (*it)->infos();

			for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = infos.begin(); !found && (jt != infos.end()); ++jt) {
				if ((*jt)->name() == pc.DefaultEncodingOption.value()) {
					ADD_OPTION_STRING("Default Encoding Set", (*it)->name().c_str());
					ADD_OPTION_STRING("Default Encoding", (*jt)->name().c_str());

					//(*jt)->visibleName().c_str());
					//myBookInfo->EncodingOption.value().c_str());

					found =true;
					break;
				}
			}
		}
		ADD_OPTION_BOOL("iso-8859-1 -> win-1251", ZLEncodingCollection::useWindows1252HackOption().value());

		cb_lcb_redraw();
	}
}

void ZLEwlOptionsDialog(FBReader &f)
{
	myFbreader = &f;
	myContext = &(*f.context());

	cb_olist *options = new cb_olist;
	olists.push_back(options);

	options->name = "Settings";
	options->parent = NULL;
	options->parent_item_idx = -1;
	options->item_handler = options_dialog_handler;

	cb_olist_item i;

	ADD_SUBMENU_ITEM("Format & Style");
	ADD_SUBMENU_ITEM("Indicator");
	ADD_SUBMENU_ITEM("Language");

	cb_lcb_new();
}
