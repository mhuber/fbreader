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

#ifndef __ZLEWLDIALOGMANAGER_H__
#define __ZLEWLDIALOGMANAGER_H__

#include <ewl/Ewl.h>

#include <stack>

#include <glib.h>

#include <ZLDialogManager.h>

class ZLEwlDialogManager : public ZLDialogManager {

public:
	static void createInstance() { ourInstance = new ZLEwlDialogManager(); }

private:
	ZLEwlDialogManager() : myWindow(0), myIsKeyboardGrabbed(false) {}

public:
	void createApplicationWindow(ZLApplication *application) const;

	shared_ptr<ZLDialog> createDialog(const ZLResourceKey &key) const;
	shared_ptr<ZLOptionsDialog> createOptionsDialog(const ZLResourceKey &id, shared_ptr<ZLRunnable> applyAction, bool showApplyButton) const;
	void informationBox(const ZLResourceKey &key, const std::string &message) const;
	void errorBox(const ZLResourceKey &key, const std::string &message) const;
	int questionBox(const ZLResourceKey &key, const std::string &message, const ZLResourceKey &button0, const ZLResourceKey &button1, const ZLResourceKey &button2) const;
	bool selectionDialog(const ZLResourceKey &key, ZLTreeHandler &handler) const;
	void wait(const ZLResourceKey &key, ZLRunnable &runnable) const;

	bool isClipboardSupported(ClipboardType type) const;
	void setClipboardText(const std::string &text, ClipboardType type) const;

	void grabKeyboard(bool grab) { myIsKeyboardGrabbed = grab; }
	bool isKeyboardGrabbed() const { return myIsKeyboardGrabbed; }

	void informationBox(const std::string&, const std::string&) const {} 
	void setClipboardImage(const ZLImageData&, ZLDialogManager::ClipboardType) const {}


private:
	int internalBox(const gchar *icon, const ZLResourceKey &key, const std::string &message, const ZLResourceKey &button0 = OK_BUTTON, const ZLResourceKey &button1 = ZLResourceKey(), const ZLResourceKey &button2 = ZLResourceKey()) const;

private:
	mutable Ewl_Window *myWindow;
	mutable std::stack<Ewl_Window*> myDialogs;
	bool myIsKeyboardGrabbed;

//friend EwlDialog *createEwlDialog(const std::string& title);
//friend void destroyEwlDialog(EwlDialog *dialog);
};

#endif /* __ZLEWLDIALOGMANAGER_H__ */
