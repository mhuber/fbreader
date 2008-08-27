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

#ifndef __ZLEWLTIME_H__
#define __ZLEWLTIME_H__

#include <map>

#include "../../../../core/src/unix/time/ZLUnixTime.h"

class ZLEwlTimeManager : public ZLUnixTimeManager {

public:
	static void createInstance() { ourInstance = new ZLEwlTimeManager(); }

	void addTask(shared_ptr<ZLRunnable> task, int interval);
	void removeTask(shared_ptr<ZLRunnable> task);

private:
	std::map<shared_ptr<ZLRunnable>,int> myHandlers;
};

#endif /* __ZLEWLTIME_H__ */
