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

#include "ZLEwlDialogManager.h"

shared_ptr<ZLDialog> ZLEwlDialogManager::createDialog(const ZLResourceKey &key) const {
}

shared_ptr<ZLOptionsDialog> ZLEwlDialogManager::createOptionsDialog(const ZLResourceKey &id, shared_ptr<ZLRunnable> applyAction, bool showApplyButton) const {
}

void ZLEwlDialogManager::informationBox(const ZLResourceKey &key, const std::string &message) const {
}

void ZLEwlDialogManager::errorBox(const ZLResourceKey &key, const std::string &message) const {
}

int ZLEwlDialogManager::questionBox(const ZLResourceKey &key, const std::string &message, const ZLResourceKey &button0, const ZLResourceKey &button1, const ZLResourceKey &button2) const {
}

int ZLEwlDialogManager::internalBox(const gchar *icon, const ZLResourceKey &key, const std::string &message, const ZLResourceKey &button0, const ZLResourceKey &button1, const ZLResourceKey &button2) const {
}

bool ZLEwlDialogManager::selectionDialog(const ZLResourceKey &key, ZLTreeHandler &handler) const {
}

void ZLEwlDialogManager::wait(const ZLResourceKey &key, ZLRunnable &runnable) const {
	runnable.run();
}

bool ZLEwlDialogManager::isClipboardSupported(ClipboardType) const {
	return false;
}

void ZLEwlDialogManager::setClipboardText(const std::string &text, ClipboardType type) const {
}
