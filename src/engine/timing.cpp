/*
    Voxel-Engine - A CPU based sparse octree renderer.
    Copyright (C) 2013  B.J. Conijn <bcmpinc@users.sourceforge.net>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "timing.h"

#define USE_QFPC // escape to use old counter

#if defined _WIN32 || defined _WIN64
#ifdef USE_QFPC
// High resolution windows timer
#include <windows.h>
#include <winbase.h>

struct TimerData {
    LARGE_INTEGER begin, end, freq;
};

Timer::Timer() : data(new TimerData())
{
#ifndef NDEBUG
	bool r = 
#endif
	QueryPerformanceFrequency(&data->freq); // obtain frequency in seconds.
	QueryPerformanceCounter(&data->begin);
}

double Timer::elapsed()
{
	QueryPerformanceCounter(&data->end);
	return (data->end.QuadPart - data->begin.QuadPart)*1000./(double)data->freq.QuadPart;
}

#else
// Low resolution windows timer
#include <timer.h>
static unsigned int time = 0;

void Timing::reset()
{
	startTime();
	time = calculateElapsedTime();
}

double Timing::elapsed()
{
	return calculateElapsedTime() - time;
}
#endif
#else
#ifdef USE_QFPC
// High resolution linux timer
#include <time.h>

struct TimerData {
    timespec begin, end, freq;
};

Timer::Timer() : data(new TimerData())
{
	clock_getres(CLOCK_MONOTONIC, &data->freq);
	//printf("Timer rsolution is %ldns\n",freq.tv_nsec);
	clock_gettime(CLOCK_MONOTONIC, &data->begin);
}

double Timer::elapsed()
{
	clock_gettime(CLOCK_MONOTONIC, &data->end);
	return (data->end.tv_sec-data->begin.tv_sec)*1000.0 + (data->end.tv_nsec-data->begin.tv_nsec)/1000000.0;
}
#else
// Low resolution linux timer
#include <time.h>
static time_t begin, end;

void Timing::reset()
{
	time(&begin);
}

double Timing::elapsed()
{
	time(&end);
	return (end - begin)* 1000.0 / CLOCKS_PER_SEC;
}
#endif
#endif

Timer::~Timer() {
    delete data;
}

