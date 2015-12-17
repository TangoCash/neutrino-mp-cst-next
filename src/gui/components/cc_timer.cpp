/*
	Based up Neutrino-GUI - Tuxbox-Project
	Copyright (C) 2001 by Steffen Hehn 'McClean'

	Generic Timer.
	Copyright (C) 2013, Thilo Graf 'dbt'

	License: GPL

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <global.h>
#include <neutrino.h>

#include "cc_timer.h"
#include <pthread.h>
#include <errno.h>
#include <system/helpers.h>
#include <system/debug.h>

using namespace std;

CComponentsTimer::CComponentsTimer( const int& interval)
{
	tm_thread 		= 0;
	tm_interval		= interval;

	sl = sigc::mem_fun(*this, &CComponentsTimer::stopTimer);
	if (interval > 0)
		startTimer();
}

CComponentsTimer::~CComponentsTimer()
{
	if (stopTimer())
		dprintf(DEBUG_INFO,"[CComponentsTimer]    [%s]  timer stopped\n", __func__);
}

//thread handle
void* CComponentsTimer::initTimerThread(void *arg)
{
	CComponentsTimer *timer = static_cast<CComponentsTimer*>(arg);

	//start loop
	while(timer) {
		timer->mutex.lock();
		timer->OnTimer();
		timer->mutex.unlock();
		mySleep(timer->tm_interval);
	}

	return 0;
}

//start up running timer with own thread, return true on succses
bool CComponentsTimer::startTimer()
{
	void *ptr = static_cast<void*>(this);

	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,0);
	pthread_setcanceltype (PTHREAD_CANCEL_ASYNCHRONOUS,0);

	if(!tm_thread) {
		int res = pthread_create (&tm_thread, NULL, initTimerThread, ptr) ;
		if (res != 0){
			dprintf(DEBUG_NORMAL,"\033[33m[CComponentsTimer] [%s - %d] ERROR! pthread_create\033[0m\n", __func__, __LINE__);
			return false;
		}
		if (res == 0){
			dprintf(DEBUG_INFO,"\033[33m[CComponentsTimer] [%s - %d]  timer thread [%lu] created with interval = %d\033[0m\n", __func__,  __LINE__, pthread_self(), tm_interval);
	CNeutrinoApp::getInstance()->OnBeforeRestart.connect(sl);
		}else{
			dprintf(DEBUG_NORMAL, "\033[33m[CComponentsTimer] [%s - %d] ERROR! pthread_create\033[0m\n", __func__, __LINE__);
		}
	}
	return  true;
}

//stop ticking timer and kill thread, return true on succses
bool CComponentsTimer::stopTimer()
{
	if(tm_thread) {
		int thres = pthread_cancel(tm_thread);
		if (thres != 0)
			dprintf(DEBUG_NORMAL,"\033[33m[CComponentsTimer] [%s - %d] ERROR! pthread_cancel\033[0m\n", __func__, __LINE__);

		thres = pthread_join(tm_thread, NULL);

		if (thres != 0)
			dprintf(DEBUG_NORMAL, "\033[33m[CComponentsTimer] [%s - %d] ERROR! pthread_join\033[0m\n", __func__, __LINE__);

	if (thres == 0){
		tm_thread = 0;
		//ensure disconnect of unused slot
		sl.disconnect();
			dprintf(DEBUG_INFO,"\033[33m[CComponentsTimer]    [%s] timer thread terminated ...\033[0m\n", __func__);
		return true;
	}
	}
	return false;
}
