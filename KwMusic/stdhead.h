#ifndef __STDHEAD_H__
#define __STDHEAD_H__

#include <Windows.h>
#include <string>
#include <vector>
#include <process.h>
#include <Psapi.h>
#include "../../API/RainmeterAPI.h"


using namespace std;


enum MeasureType
{
	MEASURE_NONE,
	MEASURE_TITLE,
	MEASURE_ARTIST,
	MEASURE_TITLEARTIST,
	MEASURE_PLAYERPATH,
	MEASURE_STATUS,
};

struct Measure
{
	//Measure() {}
	void* rm;
	MeasureType type = MEASURE_NONE;
	wstring playerpath;
};


#endif