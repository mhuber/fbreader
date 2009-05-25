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

#include <ZLFile.h>

#include "ZLEwlDialogs.h"
#include "ZLEwlChoicebox.h"
#include "ZLEwlEntry.h"
#include "ZLEwlMessage.h"
#include "virtk.h"

#include <ewl/Ewl.h>
#include <Evas.h>
#include <Edje.h>

#include <iostream>
#include <sstream>
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

#define FONT_SIZE_MIN	6
#define FONT_SIZE_MAX	24 - FONT_SIZE_MIN
#define FONT_SIZE(i) ((i)+FONT_SIZE_MIN)

static struct _action {
	char *actionId;
	char *actionName;
} actions[] = {
	"none",					"None",
	"addBookmark",			"Add Bookmark",
	"showBookmarks",		"Show Bookmarks",
	"hyperlinkNavStart",	"Hyperlinks mode",
	"showFootnotes",		"Show Footnotes",
	"gotoPageNumber",		"Go To Page",
	"toc",					"Show Table of Contents",
	"gotoHome",				"Go to Home",
	"gotoSectionStart",		"Go to Start of Section",
	"gotoSectionEnd",		"Go to End of Section",
	"nextTOCSection",		"Go to Next TOC Section",
	"previousTOCSection",	"Go to Previous TOC Section",
	"largeScrollForward",	"Large Scroll Forward",
	"largeScrollBackward",	"Large Scroll Backward",
	"undo",					"Undo",
	"redo",					"Redo",
	"search",				"Search",
	"increaseFont",			"Increase Font Size",
	"decreaseFont",			"Decrease Font Size",
	"toggleIndicator",		"Toggle Position Indicator",
//	"preferences",			"Show Options Dialog",
//	"bookInfo",				"Show Book Info Dialog",
//	"cancel",				"Cancel",
//	"quit",					"Quit",
	NULL,					NULL
};

static struct _language {
	char *langId;
	char *langName;
} languages[] = {
	"ar", "Arabic",
	"cs", "Czech",
	"de", "German",
	"de-traditional", "German (traditional orthography)",
	"el", "Greek",
	"en", "English",
	"eo", "Esperanto",
	"es", "Spanish",
	"fi", "Finnish",
	"fr", "French",
	"he", "Hebrew",
	"id", "Indonesian",
	"it", "Italian",
	"no", "Norwegian",
	"pt", "Portuguese",
	"ru", "Russian",
	"sv", "Swedish",
	"tr", "Turkish",
	"uk", "Ukrainian",
	"zh", "Chinese",
	"other", "Other",
	NULL, NULL
};

static char *alignments[] = { "undefined", "left", "right", "center", "justify" };
static char *para_break_type[] = { "New Line", "Empty Line", "Line With Indent" };
static int curBreakType = 0;
static bool reopen_file = false;

vector<cb_olist *> olists;
cb_vlist *vlist;

FBReader *myFbreader;
ZLPaintContext *myContext;
BookInfo *myBookInfo;
BookDescriptionPtr description;

static void (*next_gui)(FBReader &);

bool turbo = false;

void ZLEwlGotoPageDialog(FBReader &f)
{
	long page = read_number("Go To Page");
	if(page >= 0) {
		f.bookTextView().gotoPage(page);
		f.refreshWindow();
	}

}

static ZLTextTreeParagraph *curTOCParent;
static cb_list *list;

int toc_handler(int idx, bool is_alt)
{
	ZLTextTreeParagraph *selEntry;
	std::vector<ZLTextTreeParagraph*> toc_list;
	ContentsModel &cm = (ContentsModel&)*myFbreader->myModel->contentsModel();

	if(curTOCParent != cm.myRoot) {
		if(idx == 0)
			selEntry = curTOCParent->parent();
		else if(idx == 1)
			selEntry = curTOCParent;
		else
			selEntry = curTOCParent->children().at(idx - 2);
	} else
		selEntry = curTOCParent->children().at(idx);

	toc_list = selEntry->children();
	if(toc_list.empty() || (curTOCParent == selEntry)) {
		myFbreader->bookTextView().gotoParagraph(cm.reference(selEntry));
		myFbreader->refreshWindow();
		return 1;
	}

	curTOCParent = selEntry;

	list->items.clear();

	int cnt = 0;
	if(selEntry != cm.myRoot) {
		list->items.push_back("..");
		list->items.push_back(".");
	}

	short len;
	char *p;
	for(int i = 0; i < toc_list.size(); i++) {
		p = toc_list.at(i)->myFirstEntryAddress;

		len = 0;
		memcpy(&len, p + 3, sizeof(short));
		char *t = (char*)malloc((len + 1) * sizeof(char));
		bzero(t, len + 1);
		memcpy(t, p + 7, len);

		if(toc_list.at(i)->children().empty())
			list->items.push_back(t);
		else
			list->items.push_back(string("+") + t);
		free(t);
	}

	cb_fcb_redraw(list->items.size());
	return 0;
}

void ZLEwlTOCDialog(FBReader &f)
{
	myFbreader = &f;

	if(list)
		delete list;

	list = new cb_list;

	list->name = "Table Of Contents";
	list->alt_text = "";
	list->item_handler = toc_handler;

	short len;
	char *p;
	ContentsModel &cm = (ContentsModel&)*myFbreader->myModel->contentsModel();
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
				list->items.push_back(t);
			else
				list->items.push_back(string("+") + t);
			free(t);
		}
	}

	curTOCParent = cm.myRoot;

	cb_fcb_new(list);
}

int bookmarks_handler(int idx, bool is_alt)
{
	if(is_alt) {
		myFbreader->bookTextView().removeBookmark(idx);

		list->items.erase(list->items.begin() + idx);

		cb_fcb_redraw(list->items.size());
//		cb_fcb_invalidate_interval(idx, list->items.size() - 1);

		return 0;
	} else {
		myFbreader->bookTextView().gotoBookmark(idx);
		return 1;
	}
}

void ZLEwlBMKDialog(FBReader &f)
{
	myFbreader = &f;

	if(list)
		delete list;
	list = new cb_list;

	list->name = "Bookmarks";
	list->alt_text = "Delete";
	list->item_handler = bookmarks_handler;

	std::vector<std::pair<std::pair<int, int>, std::pair<int, std::string> > > bookmarks
		= myFbreader->bookTextView().getBookmarks();

	for(int i = 0; i < bookmarks.size(); i++) {
		stringstream s;
		s << "Page " << bookmarks.at(i).second.first << ": " << bookmarks.at(i).second.second;
		list->items.push_back(s.str());
	}

	cb_fcb_new(list);
}

void ZLEwlBMKAddedMsg(FBReader &f) {
	show_message("Bookmark added");
}

// search dialogs

void search_found_keyhandler(Evas_Object *o, char *keyname)
{
	if(!strcmp(keyname, "Escape") || !strcmp(keyname, "Return")) {
		myFbreader->myModel->bookTextModel()->removeAllMarks();
		myFbreader->clearTextCaches();
		myFbreader->refreshWindow();
		ecore_main_loop_quit();
		return;
	} else if(!strcmp(keyname, "Right") || !strcmp(keyname, "Next") || !strcmp(keyname, "Down")) {
		if(myFbreader->bookTextView().canFindNext())
			myFbreader->bookTextView().findNext();
	} else if(!strcmp(keyname, "Left") || !strcmp(keyname, "Prior") || !strcmp(keyname, "Up")) {
		if(myFbreader->bookTextView().canFindPrevious())
			myFbreader->bookTextView().findPrevious();
	}

	string s;
	bool n = false, p = false;
	if(myFbreader->bookTextView().canFindNext())
		n = true;
	if(myFbreader->bookTextView().canFindPrevious())
		p = true;

	if(n && p) 
		s = "<- Search ->";
	else if(n)
		s = "Search ->";
	else if(p)
		s = "<- Search";
	else
		s = "Search";

	edje_object_part_text_set(o, "text", s.c_str());
}

void search_not_found_message(FBReader &f)
{
	show_message("Text not found");
}

void search_found_message(FBReader &f)
{
	bool n = false, p = false;

	if(myFbreader->bookTextView().canFindNext())
		n = true;
	if(myFbreader->bookTextView().canFindPrevious())
		p = true;

	if(n && p) 
		show_message("<- Search ->", (void*)search_found_keyhandler);
	else if(n)
		show_message("Search ->", (void*)search_found_keyhandler);
	else if(p)
		show_message("<- Search", (void*)search_found_keyhandler);
	else
		show_message("Search");
}

void search_input_handler(char *text)
{
	if(text && strlen(text)) {
		myFbreader->clearTextCaches();
		myFbreader->refreshWindow();

		if(myFbreader->bookTextView().search(std::string(text), true, false, false, false))
			next_gui = search_found_message;
		else
			next_gui = search_not_found_message;
	}
}

void ZLEwlSearchDialog(FBReader &f)
{
	Ewl_Widget *w;
	myFbreader = &f;

	next_gui = NULL;

	ewl_widget_show(w = init_virtk(NULL, "Search", search_input_handler));
	ecore_main_loop_begin();
	if(w) {
		ewl_widget_hide(w);
		ewl_widget_destroy(w);
	}

	// run next gui window
	if(next_gui) {
		myFbreader->refreshWindow();
		next_gui(f);
	}
}

void settings_close_handler()
{
	if(reopen_file)
		myFbreader->openFile(myFbreader->myModel->fileName());

	reopen_file = false;

	myFbreader->clearTextCaches();
//    myFbreader->refreshWindow();
}

void ZLBooleanOption_handler(int idx, bool is_alt)
{
	cb_olist_item *i = &olists.back()->items.at(idx);
	++i->curval_idx %= i->values.size();

	ZLBooleanOption *o = (ZLBooleanOption*)i->data;
	o->setValue(i->values.at(i->curval_idx).ibool);

	cb_lcb_invalidate(idx);
}

void font_family_handler(int idx, bool is_alt)
{
	ZLStringOption &option = ZLTextStyleCollection::instance().baseStyle().FontFamilyOption;
	option.setValue(myContext->fontFamilies().at(idx));
	
	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.text = iv.sval = option.value();

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void line_spacing_handler(int idx, bool is_alt)
{
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().LineSpacePercentOption;
	option.setValue(idx * 10 + 50);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();
	char *t;
	asprintf(&t, "%d%%", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void font_size_handler(int idx, bool is_alt)
{
	ZLIntegerRangeOption &option = ZLTextStyleCollection::instance().baseStyle().FontSizeOption;
	option.setValue(FONT_SIZE(idx));

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();

	char *t;
	asprintf(&t, "%dpt", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void indicator_offset_handler(int idx, bool is_alt)
{
	ZLIntegerRangeOption &option = FBView::commonIndicatorInfo().OffsetOption;

	option.setValue(idx * 5);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();

	char *t;
	asprintf(&t, "%d", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void indicator_height_handler(int idx, bool is_alt)
{
	ZLIntegerRangeOption &option = FBView::commonIndicatorInfo().HeightOption;

	if(!idx)
		option.setValue(1);
	else
		option.setValue(idx * 5);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();

	char *t;
	asprintf(&t, "%d", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void indicator_font_size_handler(int idx, bool is_alt)
{
	ZLIntegerRangeOption &option = FBView::commonIndicatorInfo().FontSizeOption;
	option.setValue(FONT_SIZE(idx));

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();

	char *t;
	asprintf(&t, "%dpt", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void alignment_handler(int idx, bool is_alt)
{
	ZLIntegerOption &option = ZLTextStyleCollection::instance().baseStyle().AlignmentOption;
	option.setValue(idx+1);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();
	iv.text = alignments[iv.ival];

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void margins_val_handler(int idx, bool is_alt)
{
	FBMargins &margins = FBView::margins();
	ZLIntegerRangeOption *option;

	switch(vlist->parent_item_idx) {
		case 0:
			option = &margins.LeftMarginOption;
			break;
		case 1:
			option = &margins.RightMarginOption;
			break;
		case 2:
			option = &margins.TopMarginOption;
			break;
		case 3:
			option = &margins.BottomMarginOption;
			break;
	}

	option->setValue(idx * 5);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option->value();

	char *t;
	asprintf(&t, "%d", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void margins_handler(int idx, bool is_alt)
{
	string t;

	switch(idx) {
		case 0:
			t = "Left Margin";
			break;
		case 1:
			t = "Right Margin";
			break;
		case 2:
			t = "Top Margin";
			break;
		case 3:
			t = "Bottom Margin";
			break;
	}

	INIT_VLIST(t, margins_val_handler);

	for(int i = 0; i <= 100; i+=5)
		ADD_VALUE_INT(i);

	cb_rcb_new();
}

void first_line_indent_handler(int idx, bool is_alt)
{
	ZLIntegerRangeOption &option = ((ZLTextFullStyleDecoration*)ZLTextStyleCollection::instance().decoration(/*REGULAR*/0))->FirstLineIndentDeltaOption;
	option.setValue(idx * 5);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.ival = option.value();

	char *t;
	asprintf(&t, "%d", iv.ival);
	iv.text = t;
	free(t);

	cb_lcb_invalidate(vlist->parent_item_idx);

}

void format_style_handler(int idx, bool is_alt)
{
	cb_olist *current_olist = olists.back();
	if(0 == idx) {
		INIT_VLIST("Font Family", font_family_handler);

		for(int i = 0; i < myContext->fontFamilies().size(); i++)
			ADD_VALUE_STRING(myContext->fontFamilies().at(i).c_str());
		
		cb_rcb_new();
	} else if(1 == idx) {
		INIT_VLIST("Font Size", font_size_handler);

		for(int i = 0; i <= FONT_SIZE_MAX; i++)
			ADD_VALUE_INT_F(FONT_SIZE(i), "%dpt");
		
		cb_rcb_new();
	} else if(3 == idx) {
		INIT_VLIST("Line Spacing", line_spacing_handler);

		for(int i = 50; i <= 200; i+=10)
			ADD_VALUE_INT_F(i, "%d%%");
		
		cb_rcb_new();
	} else if(4 == idx) {
		INIT_VLIST("Alignment", alignment_handler);

		for(int i = 1; i < 5; i++)
			ADD_VALUE_STRING(alignments[i]);
		
		cb_rcb_new();
	} else if(5 == idx) {
		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Margins";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = margins_handler;
		options->destroy_handler = NULL;

		cb_olist_item i;

		FBMargins &margins = FBView::margins();

		ADD_OPTION_INT("Left Margin", margins.LeftMarginOption.value());
		ADD_OPTION_INT("Right Margin", margins.RightMarginOption.value());
		ADD_OPTION_INT("Top Margin", margins.TopMarginOption.value());
		ADD_OPTION_INT("Bottom Margin", margins.BottomMarginOption.value());

		cb_lcb_redraw();
	} else if(6 == idx) {
		INIT_VLIST("First Line Indent", first_line_indent_handler);

		for(int i = 0; i <= 100; i += 5)
			ADD_VALUE_INT(i);
		
		cb_rcb_new();
	}
}

void indicator_handler(int idx, bool is_alt)
{
	cb_olist *current_olist = olists.back();

	FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();

	if(0 == idx) {
		indicatorInfo.TypeOption.setValue(
				(indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR) ?
				ZLTextPositionIndicatorInfo::NONE : ZLTextPositionIndicatorInfo::FB_INDICATOR);

		cb_olist_item *i = &olists.back()->items.at(idx);
		++i->curval_idx %= i->values.size();

		cb_lcb_invalidate(idx);
	} else if(5 == idx) {
		INIT_VLIST("Indicator Height", indicator_height_handler);

		for(int i = 1; i <= 30; i == 1 ? i = 5 : i += 5)
			ADD_VALUE_INT(i);
		
		cb_rcb_new();
	} else if(6 == idx) {
		INIT_VLIST("Offset From Text", indicator_offset_handler);

		for(int i = 0; i <= 100; i+=5)
			ADD_VALUE_INT(i);
		
		cb_rcb_new();
	} else if(7 == idx) {
		INIT_VLIST("Indicator Font Size", indicator_font_size_handler);

		for(int i = 0; i <= 16 - FONT_SIZE_MIN; i++)
			ADD_VALUE_INT_F(FONT_SIZE(i), "%dpt");
		
		cb_rcb_new();
	}
}

void default_language_handler(int idx, bool is_alt)
{
	int lsize = sizeof(languages) / sizeof(struct _language);

	ZLStringOption &option = PluginCollection::instance().DefaultLanguageOption;
	if(idx < lsize-1) {
		option.setValue(languages[idx].langId);

		cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
		iv.text = iv.sval = languages[idx].langName;

		cb_lcb_invalidate(vlist->parent_item_idx);
	}
}

void default_encoding_set_handler(int idx, bool is_alt)
{
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	if(idx > sets.size())
		return;

	cb_item_value &iv1 = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv1.text = iv1.sval = sets.at(idx)->name();
	cb_lcb_invalidate(vlist->parent_item_idx);

	PluginCollection::instance().DefaultEncodingOption.setValue(sets.at(idx)->infos().at(0)->name());

	cb_item_value &iv2 = olists.back()->items.at(vlist->parent_item_idx + 1).current_value;
	iv2.text = iv2.sval = sets.at(idx)->infos().at(0)->name();
	cb_lcb_invalidate(vlist->parent_item_idx + 1);
}

void default_encoding_handler(int idx, bool is_alt)
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

	if(idx >= pinfos->size())
		return;

	std::string newenc = (*pinfos).at(idx)->name();
	PluginCollection::instance().DefaultEncodingOption.setValue(newenc);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.text = iv.sval = newenc;

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void language_handler(int idx, bool is_alt)
{
	if(1 == idx) {
		INIT_VLIST("Default Language", default_language_handler);

		for(unsigned int i = 0; i < sizeof(languages) / sizeof(struct _language) && languages[i].langId; i++)
			ADD_VALUE_STRING(languages[i].langName);
		
		cb_rcb_new();
	} else if(2 == idx) {
		const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
		INIT_VLIST("Default Encoding Set", default_encoding_set_handler);

		for(unsigned int i = 0; i < sets.size(); i++)
			ADD_VALUE_STRING(sets.at(i)->name());

		cb_rcb_new();
	} else if(3 == idx) {
		INIT_VLIST("Default Encoding", default_encoding_handler);

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

		for(unsigned int i = 0; i < infos.size(); i++) 
			ADD_VALUE_STRING(infos.at(i)->visibleName());

		cb_rcb_new();
	}
}

void book_encoding_set_handler(int idx, bool is_alt)
{
	const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
	if(idx > sets.size())
		return;

	std::string newenc = sets.at(idx)->infos().at(0)->name();

	BookDescriptionPtr description = new BookDescription(myFbreader->myModel->fileName());
	description->myEncoding = newenc;
	BookDescriptionUtil::saveInfo(ZLFile(myFbreader->myModel->fileName()));

	myBookInfo->EncodingOption.setValue(newenc);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.text = iv.sval = sets.at(idx)->name();
	cb_lcb_invalidate(vlist->parent_item_idx);

	cb_item_value &iv2 = olists.back()->items.at(vlist->parent_item_idx + 1).current_value;
	iv2.text = iv2.sval = newenc;
	cb_lcb_invalidate(vlist->parent_item_idx + 1);

	reopen_file = true;
}

void book_encoding_handler(int idx, bool is_alt)
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

	if(idx >= pinfos->size())
		return;

	std::string newenc = (*pinfos).at(idx)->name();

	BookDescriptionPtr description = new BookDescription(myFbreader->myModel->fileName());
	description->myEncoding = newenc;
	BookDescriptionUtil::saveInfo(ZLFile(myFbreader->myModel->fileName()));

	myBookInfo->EncodingOption.setValue(newenc);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.text = iv.sval = newenc;
	cb_lcb_invalidate(vlist->parent_item_idx);

	reopen_file = true;
}

void book_language_handler(int idx, bool is_alt)
{
	int lsize = sizeof(languages) / sizeof(struct _language);

	BookDescriptionPtr description = new BookDescription(myFbreader->myModel->fileName());
	description->myLanguage = languages[idx].langId;

	BookDescriptionUtil::saveInfo(ZLFile(myFbreader->myModel->fileName()));

	// set encoding
	myBookInfo->LanguageOption.setValue(description->myLanguage);

	cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
	iv.text = iv.sval = languages[idx].langName; //(description->myLanguage);
	cb_lcb_invalidate(vlist->parent_item_idx);

	reopen_file = true;
}

void book_para_break_handler(int idx, bool is_alt)
{
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

			idx << 1;
			idx++;
			idx %= 5;

			myFormat.BreakTypeOption.setValue(idx);

			switch (myFormat.BreakTypeOption.value()) {
				case PlainTextFormat::BREAK_PARAGRAPH_AT_NEW_LINE:
					idx = 0;
					break;
				case PlainTextFormat::BREAK_PARAGRAPH_AT_EMPTY_LINE:
					idx = 1;
					break;
				case PlainTextFormat::BREAK_PARAGRAPH_AT_EMPTY_LINE | PlainTextFormat::BREAK_PARAGRAPH_AT_LINE_WITH_INDENT:
				default:
					idx = 2;
			}

			cb_item_value &iv = olists.back()->items.at(vlist->parent_item_idx).current_value;
			iv.text = para_break_type[idx];
			iv.ival = idx;

			cb_lcb_invalidate(vlist->parent_item_idx);
			reopen_file = true;
		}
	}
}

void book_settings_handler(int idx, bool is_alt)
{
	if(0 == idx) {
		INIT_VLIST("Book Language", book_language_handler);

		for(unsigned int i = 0; i < sizeof(languages) / sizeof(struct _language) && languages[i].langId; i++)
			ADD_VALUE_STRING(languages[i].langName);
		
		cb_rcb_new();
	} 

	if((myBookInfo->EncodingOption.value() != "auto") && ((1 == idx) || (2 == idx))) {
		if(1 == idx) {
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			INIT_VLIST("Default Encoding Set", book_encoding_set_handler);

			for(unsigned int i = 0; i < sets.size(); i++)
				ADD_VALUE_STRING(sets.at(i)->name());

			cb_rcb_new();
		} else if(2 == idx) {
			INIT_VLIST("Default Encoding", book_encoding_handler);

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

			for(unsigned int i = 0; i < infos.size(); i++) 
				ADD_VALUE_STRING(infos.at(i)->visibleName());

			cb_rcb_new();
		}
	}

	if(((myBookInfo->EncodingOption.value() != "auto") && (3 == idx)) ||
		((myBookInfo->EncodingOption.value() == "auto") && (2 == idx))) {

		INIT_VLIST("Break Paragraph At", book_para_break_handler);

		for(unsigned int i = 0; i < 3; i++)
			ADD_VALUE_INT_T(i, para_break_type[i]);

		cb_rcb_new();
	}
}

void single_key_handler(int idx, bool is_alt)
{
	if(!actions[idx].actionId)
		return;

	ZLKeyBindings *kb = &(*myFbreader->myBindings0);
	stringstream k;
	k << (vlist->parent_item_idx+1);

	kb->bindKey(k.str(), actions[idx].actionId);
	olists.back()->items.at(vlist->parent_item_idx).name = actions[idx].actionName;

	cb_lcb_invalidate(vlist->parent_item_idx);
}

void keys_handler(int idx, bool is_alt)
{
	char *k;
	asprintf(&k, "Key %d", idx + 1);

	INIT_VLIST(k, single_key_handler);

	for(struct _action *a = actions; a->actionName; a++)
		ADD_VALUE_STRING(a->actionName);

	cb_rcb_new();
}

void options_dialog_handler(int idx, bool is_alt)
{
//	fprintf(stderr, "options_dialog_handler: %d\n", idx);

	cb_olist *current_olist = olists.back();

	if(0 == idx) {
		myContext = &(*myFbreader->context());

		ZLTextBaseStyle &bs = ZLTextStyleCollection::instance().baseStyle();

		ZLTextStyleCollection &collection = ZLTextStyleCollection::instance();
		ZLTextFullStyleDecoration *decoration = (ZLTextFullStyleDecoration*)collection.decoration(/*REGULAR*/0);

		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Format & Style";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = format_style_handler;
		options->destroy_handler = NULL;

		cb_olist_item i;

		ADD_OPTION_STRING(	"Font Family", bs.FontFamilyOption.value());
		ADD_OPTION_INT_F(	"Font Size", bs.FontSizeOption.value(), "%dpt");
		ADD_OPTION_BOOL_H(	"Bold", bs.BoldOption.value(), ZLBooleanOption_handler, &bs.BoldOption);
		ADD_OPTION_INT_F(	"Line Spacing", bs.LineSpacePercentOption.value(), "%d%%");
		ADD_OPTION_INT_T(	"Alignment", bs.AlignmentOption.value(), alignments[bs.AlignmentOption.value()]);
		ADD_OPTION_STRING(	"Margins", "");
		ADD_OPTION_INT(		"First Line Indent", decoration->FirstLineIndentDeltaOption.value());
		ADD_OPTION_BOOL_H(	"Auto Hyphenations", bs.AutoHyphenationOption.value(), ZLBooleanOption_handler, &bs.AutoHyphenationOption);

		cb_lcb_redraw();
	} else if(1 == idx) {
		FBIndicatorStyle &indicatorInfo = FBView::commonIndicatorInfo();

		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Indicator";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = indicator_handler;
		options->destroy_handler = NULL;

		cb_olist_item i;

		ADD_OPTION_BOOL(	"Show Indicator", (indicatorInfo.TypeOption.value() == ZLTextPositionIndicatorInfo::FB_INDICATOR));
		ADD_OPTION_BOOL_H(	"Show TOC Marks", myFbreader->bookTextView().ShowTOCMarksOption.value(), ZLBooleanOption_handler, &myFbreader->bookTextView().ShowTOCMarksOption);
		ADD_OPTION_BOOL_H(	"Show Position", indicatorInfo.ShowTextPositionOption.value(), ZLBooleanOption_handler, &indicatorInfo.ShowTextPositionOption);
		ADD_OPTION_BOOL_H(	"Show Time", indicatorInfo.ShowTimeOption.value(), ZLBooleanOption_handler, &indicatorInfo.ShowTimeOption);
		ADD_OPTION_BOOL_H(	"Show Battery", indicatorInfo.ShowBatteryOption.value(), ZLBooleanOption_handler, &indicatorInfo.ShowBatteryOption);
		ADD_OPTION_INT(		"Indicator Height", indicatorInfo.HeightOption.value());
		ADD_OPTION_INT(		"Offset from Text", indicatorInfo.OffsetOption.value());
		ADD_OPTION_INT_F(	"Font Size", indicatorInfo.FontSizeOption.value(), "%dpt");

		cb_lcb_redraw();
	} else if(2 == idx) {
		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Language";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = language_handler;
		options->destroy_handler = NULL;

		PluginCollection &pc = PluginCollection::instance();

		cb_olist_item i;


		ADD_OPTION_BOOL_H("Detect Language and Encoding", pc.LanguageAutoDetectOption.value(), ZLBooleanOption_handler, &pc.DefaultLanguageOption);

		char *l;
		for(unsigned int i = 0; i < sizeof(languages) / sizeof(struct _language) && languages[i].langId; i++) {
			l = languages[i].langName;
			if(!pc.DefaultLanguageOption.value().compare(languages[i].langId))
				break;
		}

		ADD_OPTION_STRING("Default Language", l);

		bool found = false;
		const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
		for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
			const std::vector<ZLEncodingConverterInfoPtr> &infos = (*it)->infos();

			for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = infos.begin(); !found && (jt != infos.end()); ++jt) {
				if ((*jt)->name() == pc.DefaultEncodingOption.value()) {
					ADD_OPTION_STRING("Default Encoding Set", (*it)->name().c_str());
					ADD_OPTION_STRING("Default Encoding", (*jt)->name().c_str());

					found = true;
					break;
				}
			}
		}
		ADD_OPTION_BOOL_H("iso-8859-1 -> win-1251", ZLEncodingCollection::useWindows1252HackOption().value(), ZLBooleanOption_handler, &ZLEncodingCollection::useWindows1252HackOption());

		cb_lcb_redraw();
	} else if(3 == idx) {
		const std::string &fileName = myFbreader->myModel->fileName();
		myBookInfo = new BookInfo(fileName);

		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Book Settings";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = book_settings_handler;
		options->destroy_handler = NULL;

		cb_olist_item i;

		char *l;
		for(unsigned int j = 0; j < sizeof(languages) / sizeof(struct _language) && languages[j].langId; j++) {
			l = languages[j].langName;
			if(!myBookInfo->LanguageOption.value().compare(languages[j].langId))
				break;
		}

		ADD_OPTION_STRING("Language", l);

		if(myBookInfo->EncodingOption.value() == "auto") {
			ADD_OPTION_STRING("Encoding", myBookInfo->EncodingOption.value().c_str());
		} else {
			bool found = false;
			const std::vector<shared_ptr<ZLEncodingSet> > &sets = ZLEncodingCollection::instance().sets();
			for (std::vector<shared_ptr<ZLEncodingSet> >::const_iterator it = sets.begin(); !found && (it != sets.end()); ++it) {
				const std::vector<ZLEncodingConverterInfoPtr> &infos = (*it)->infos();

				for (std::vector<ZLEncodingConverterInfoPtr>::const_iterator jt = infos.begin(); !found && (jt != infos.end()); ++jt) {
					if ((*jt)->name() == myBookInfo->EncodingOption.value()) {
						ADD_OPTION_STRING("Encoding Set", (*it)->name().c_str());
						ADD_OPTION_STRING("Encoding", (*jt)->name().c_str());

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

//				int curBreakType;
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

				ADD_OPTION_INT_T("Break Paragraph at", curBreakType, para_break_type[curBreakType]);
			}
		}
		cb_lcb_redraw();
	} else if(4 == idx) {
		cb_olist *options = new cb_olist;
		olists.push_back(options);

		options->name = "Keys";
		options->parent = current_olist;
		options->parent_item_idx = idx;
		options->item_handler = keys_handler;
		options->destroy_handler = NULL;

		cb_olist_item i;

		struct _action *a;
		ZLKeyBindings *kb = &(*myFbreader->myBindings0);
		for(int k = 1; k <= 9; k++) {
			stringstream s;
			s << k;

			for(a = actions; a->actionId && kb->getBinding(s.str()).compare(a->actionId); a++)
				;

			ADD_OPTION_STRING(a->actionName ? a->actionName : kb->getBinding(s.str()), "");
		}

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
	options->destroy_handler = settings_close_handler;

	cb_olist_item i;

	ADD_SUBMENU_ITEM("Format & Style");
	ADD_SUBMENU_ITEM("Indicator");
	ADD_SUBMENU_ITEM("Language");
	ADD_SUBMENU_ITEM("Book settings");
	ADD_SUBMENU_ITEM("Keys");

	cb_lcb_new();
}

void ZLEwlBookInfo(FBReader &f)
{
	myFbreader = &f;

	if(list)
		delete list;
	list = new cb_list;

	list->name = "Book Info";
	list->alt_text = "";
	list->item_handler = NULL;

	const std::string &fileName = f.myModel->fileName();
	myBookInfo = new BookInfo(fileName);

#define list_add_s(__t1__, __t2__) \
	{ \
	stringstream s; \
	s << (__t1__) << (__t2__);	\
	list->items.push_back(s.str());	\
	}

	list_add_s("File: ", ZLFile::fileNameToUtf8(ZLFile(fileName).name(false)));
	list_add_s("Full path: ", ZLFile::fileNameToUtf8(ZLFile(fileName).path()));
	list_add_s("Title: ", myBookInfo->TitleOption.value());
	list_add_s("Author: ", myBookInfo->AuthorDisplayNameOption.value());

	if(!myBookInfo->SeriesNameOption.value().empty()) {
		list_add_s("Series: ", myBookInfo->SeriesNameOption.value());
		list_add_s("Book number: ", myBookInfo->NumberInSeriesOption.value());
	}

	cb_fcb_new(list);
}

int mmenu_handler(int idx, bool is_alt)
{
	switch(idx) {
		case 0:
			next_gui = ZLEwlBookInfo;
			return 1;
			break;
		case 1:
			next_gui = ZLEwlGotoPageDialog;
			return 1;
			break;
		case 2:
			next_gui = ZLEwlTOCDialog;
			return 1;
			break;
		case 3:
			next_gui = ZLEwlSearchDialog;
			return 1;
			break;
		case 4:
			next_gui = ZLEwlBMKDialog;
			return 1;
			break;
/*		case 5:
			next_gui = ZLEwlOptionsDialog;
			return 1;
*/			
		default:
			return 0;
	}

	//cb_fcb_redraw(list->items.size());
	return 0;
}

void ZLEwlMainMenu(FBReader &f)
{
	myFbreader = &f;
	myContext = &(*f.context());

	next_gui = NULL;

	if(list)
		delete list;

	list = new cb_list;

	list->name = "Main Menu";
	list->alt_text = "";
	list->item_handler = mmenu_handler;

	list->items.push_back("Book Info");
	list->items.push_back("Go To Page");
	list->items.push_back("Table Of Contents");
	list->items.push_back("Search");
	list->items.push_back("Bookmarks");
//	list->items.push_back("Settings");

	cb_fcb_new(list);

	// run next gui window
	if(next_gui) {
		myFbreader->refreshWindow();
		next_gui(f);
	}
}
