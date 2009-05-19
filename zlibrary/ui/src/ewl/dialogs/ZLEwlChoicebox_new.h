#ifndef ZLEWLCHOICEBOX_NEW_H
#define ZLEWLCHOICEBOX_NEW_H

using namespace std;

enum cb_olist_item_type {
	ITEM_TEXT,
	ITEM_SUBMENU,
	ITEM_OPTION
};

enum cb_value_type {
	VAL_INT,
	VAL_BOOL,
	VAL_STRING,
	VAL_NONE
};

typedef struct _cb_olist_item cb_olist_item;
typedef struct _cb_olist cb_olist;
typedef struct _cb_item_value cb_item_value;

struct _cb_item_value {
	cb_value_type type;
	int ival;
	bool ibool;
	string sval;
	string text;
};

struct _cb_olist_item {
	string name;
	cb_olist_item_type type;

	cb_item_value current_value;
	vector<cb_item_value> values;
	int curval_idx;
};

struct _cb_olist {
	string name;
	vector<cb_olist_item> items;
	cb_olist *parent;
	int parent_item_idx;
	void(*item_handler)(int);
};

typedef struct _cb_vlist cb_vlist;

struct _cb_vlist {
	string name;
	vector<cb_item_value> values;
	cb_olist *parent;
	int parent_item_idx;
	void(*item_handler)(int);
};

void cb_lcb_new();
void cb_lcb_redraw();
void cb_lcb_invalidate(int idx);

extern vector<cb_olist *> olists;
extern cb_vlist *vlist;

#define ADD_OPTION_STRING(__name, __value) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.text = i.current_value.sval = (__value);	\
		i.current_value.type = VAL_STRING;	\
		options->items.push_back(i);

#define ADD_OPTION_INT_T(__name, __value, __valuetext) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.text = (__valuetext); \
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		options->items.push_back(i);

#define ADD_OPTION_INT(__name, __value) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		{	\
			char *t;	\
			asprintf(&t, "%d", __value);	\
			i.current_value.text = t; \
			free(t);	\
		}	\
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		options->items.push_back(i);


#define ADD_OPTION_INT_F(__name, __value, __format) \
		i.name = (__name);	\
		i.type = ITEM_OPTION;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		{	\
			char *t;	\
			asprintf(&t, (__format), __value);	\
			i.current_value.text = t; \
			free(t);	\
		}	\
		i.current_value.ival = (__value);	\
		i.current_value.type = VAL_INT;	\
		options->items.push_back(i);

#define ADD_OPTION_BOOL(__name, __value) \
		i.name = (__name); 	\
		i.type = ITEM_OPTION;	\
		{	\
			i.values.clear();	\
			cb_item_value iv;	\
			iv.type = VAL_BOOL;	\
			iv.text = "On";	\
			iv.ibool = true;	\
			i.values.push_back(iv);	\
			iv.text = "Off";	\
			iv.ibool = false;	\
			i.values.push_back(iv);	\
			if((__value))	\
				i.curval_idx = 0;	\
			else	\
				i.curval_idx = 1;	\
		}	\
		options->items.push_back(i);

#define ADD_SUBMENU_ITEM(__name) \
		i.name = (__name);	\
		i.type = ITEM_SUBMENU;	\
		i.values.clear();	\
		i.curval_idx = -1;	\
		i.current_value.type = VAL_NONE;	\
		options->items.push_back(i);

#endif
