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

#include "ZLEwlUtil.h"

#include <iostream>
#include <csignal>

bool idle;
timer_t t_id;

void suspend(int signo, siginfo_t* evp, void* ucontext)
{
	return;
	if(!idle)
		return;

	FILE *state = fopen("/sys/power/state", "w");
	if(state) {
		fputs("mem", state);
		fclose(state);
	}
}

void init_timer()
{
	return;

	struct sigaction sigv;
	struct sigevent sigx;

	sigemptyset(&sigv.sa_mask);
	sigv.sa_flags = SA_SIGINFO;
	sigv.sa_sigaction = suspend;

	if(sigaction(SIGUSR1, &sigv, 0) == -1) {
		perror("sigaction");
		return;
	}

	sigx.sigev_notify = SIGEV_SIGNAL;
	sigx.sigev_signo = SIGUSR1;
	sigx.sigev_value.sival_ptr = (void *)NULL;

	if(timer_create(CLOCK_REALTIME, &sigx, &t_id) == -1) {
		perror("timer_create");
		return;
	}
}

void delete_timer()
{
	return;
	timer_delete(t_id);
}

void set_timer()
{
	return;
	struct itimerspec val;
	
	val.it_value.tv_sec = 4;
	val.it_value.tv_nsec = 0;
	val.it_interval.tv_sec = 0;
	val.it_interval.tv_nsec = 0;

	idle = true;
	timer_settime(t_id, 0, &val, 0);
}

void busy()
{
	return;
	idle = false;
}
