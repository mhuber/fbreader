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

#include <ZLApplication.h>
#include <ZLibrary.h>

#include "../../../../core/src/unix/library/ZLibraryImplementation.h"

#include "../filesystem/ZLEwlFSManager.h"
#include "../time/ZLEwlTime.h"
#include "../dialogs/ZLEwlDialogManager.h"
#include "../image/ZLEwlImageManager.h"
#include "../view/ZLEwlPaintContext.h"
#include "../util/ZLEwlUtil.h"
#include "../../unix/message/ZLUnixMessage.h"
#include "../../../../core/src/util/ZLKeyUtil.h"
#include "../../../../core/src/unix/xmlconfig/XMLConfig.h"
#include "../../../../core/src/unix/iconv/IConvEncodingConverter.h"

extern xcb_connection_t *connection;

class ZLEwlLibraryImplementation : public ZLibraryImplementation {

private:
	void init(int &argc, char **&argv);
	ZLPaintContext *createContext();
	void run(ZLApplication *application);
};

void initLibrary() {
	new ZLEwlLibraryImplementation();
}

void ZLEwlLibraryImplementation::init(int &argc, char **&argv) {
	if(!ewl_init(&argc, argv)) {
		fprintf(stderr, "Unable to init EWL.\n");
	}		

	ZLibrary::parseArguments(argc, argv);

	XMLConfigManager::createInstance();
	ZLEwlFSManager::createInstance();
	ZLEwlTimeManager::createInstance();
	ZLEwlDialogManager::createInstance();
	ZLUnixCommunicationManager::createInstance();
	ZLEwlImageManager::createInstance();
	ZLEncodingCollection::instance().registerProvider(new IConvEncodingConverterProvider());

	//ZLKeyUtil::setKeyNamesFileName("keynames-xcb.xml");
}

ZLPaintContext *ZLEwlLibraryImplementation::createContext() {
	ZLEwlPaintContext *pc = new ZLEwlPaintContext();
	return (ZLPaintContext *)pc;
}

static struct {
	int keynum;
	char *keyname;
} _keys[] = {
	9, "Escape",
	10, "1",
	11, "2",
	12, "3",
	13, "4",
	14, "5",
	15, "6",
	16, "7",
	17, "8",
	18, "9",
	19, "0",
	36, "Return",
	64, "Alt_L",
	65, " ",
	82, "-",
	86, "+",
	119, "Delete",
	111, "Up",
	112, "Prior",
	113, "Left",
	114, "Right",
	116, "Down",
	117, "Next",
	124, "XF86PowerOff",
	147, "Menu",
	161, "XF86RotateWindows",
	172, "XF86AudioPlay",
	225, "XF86Search",
	0, NULL
};

bool _fbreader_closed;

void main_loop(ZLApplication *application)
{
	xcb_generic_event_t  *e;
	static bool alt_pressed = false;

	std::map<int, std::string> kmap;
	int i = 0;

	while(_keys[i].keynum) {
		kmap.insert(std::make_pair(_keys[i].keynum, _keys[i].keyname));
		i++;
	}


//	init_timer();

	_fbreader_closed = false;
	while (!_fbreader_closed) {
//		set_timer();
		e = xcb_wait_for_event(connection);
//		busy();
		if (e) {
			switch (e->response_type & ~0x80) {
				case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *ev = (xcb_key_press_event_t *)e;

						if(!alt_pressed && kmap[ev->detail] == "ALT_L")
							alt_pressed = true;

						break;
					}
				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev = (xcb_key_release_event_t *)e;

						if(alt_pressed && kmap[ev->detail] == "ALT_L") {
							alt_pressed = false;
							continue;
						}

						printf("ev->detail: %d %s\n", ev->detail, kmap[ev->detail].c_str());

						if(alt_pressed)
							application->doActionByKey(std::string("Alt+") + kmap[ev->detail]);
						else
							application->doActionByKey(kmap[ev->detail]);

//						application->doActionByKey(ZLKeyUtil::keyName(ev->detail, ev->detail, ev->state));


/*						switch (ev->detail) {
							// ESC
							case 9:
								application->doAction(ActionCode::CANCEL);
								end = true;
								break;

							case 19:
							case 90:
							case 111:
								application->doAction(ActionCode::LARGE_SCROLL_FORWARD);
								break;

							case 18:
							case 81:
							case 116:
								application->doAction(ActionCode::LARGE_SCROLL_BACKWARD);
								break;
						}
*/
					}
			}
			free (e);
		}
	}
	//delete_timer();
}

void ZLEwlLibraryImplementation::run(ZLApplication *application) {
	ZLDialogManager::instance().createApplicationWindow(application);
	application->initWindow();
	main_loop(application);
	delete application;
}
