#pragma once

#include "StdAfx.h"
#include "Player.h"


enum PlayerName
{
	PLAYER_KUWO,
	PLAYER_KUGOU,
	PLAYER_QQ,
	PLAYER_BAIDU,
};

enum MeasureType
{
	MEASURE_NONE,
	MEASURE_TITLE,
	MEASURE_ARTIST,
	MEASURE_TRACK,
	MEASURE_PLAYERPATH,
	MEASURE_STATUS,
	MEASURE_COVER,
};

struct Measure
{
	Player* player;
	void* rm;

	PlayerName name;
	MeasureType type = MEASURE_NONE;

	wstring playerpath;
	wstring trackChangeAction;
};





