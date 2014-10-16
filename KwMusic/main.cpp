#include "stdhead.h"
#include "PlayerKwMusic.h"

vector<Measure*> g_Measures;
int g_mCount = 0;
extern PlayerKwMusic* g_KwData;
//extern double debug;

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
	measure->rm = rm;

	g_Measures.push_back(measure);

	g_mCount++;

	if (g_mCount == 1)
	{
		g_KwData = new PlayerKwMusic;
		g_KwData->Initialize();

	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;

	vector<Measure*>::iterator iter = find(
		g_Measures.begin(), 
		g_Measures.end(), 
		measure);

	g_Measures.erase(iter);

	delete measure;

	g_mCount--;

	if (g_mCount == 0)
	{
		delete g_KwData;
	}
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	LPCWSTR str = RmReadString(rm, L"PlayerType", L"");

	if (_wcsicmp(L"TITLE", str) == 0)
	{
		measure->type = MEASURE_TITLE;
	}
	else if (_wcsicmp(L"ARTIST", str) == 0)
	{
		measure->type = MEASURE_ARTIST;
	}
	else if (_wcsicmp(L"TITLEARTIST", str) == 0)
	{
		measure->type = MEASURE_TITLEARTIST;
	}
	else if (_wcsicmp(L"PLAYERPATH", str) == 0)
	{
		measure->type = MEASURE_PLAYERPATH;
		measure->playerpath = RmReadString(rm, L"PlayerPath", L"");
	}
	else if (_wcsicmp(L"STATUS", str) == 0)
	{
		measure->type = MEASURE_STATUS;
	}
	else 
	{
		std::wstring error = L"KwMusic.dll: Invalid PlayerType=";
		error += str;
		error += L" in [";
		error += RmGetMeasureName(rm);
		error += L"]";
		RmLog(LOG_WARNING, error.c_str());
	}
	
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;

	if (measure->type == MEASURE_STATUS)
		return g_KwData->GetStatus();
	//if (measure->type == MEASURE_NONE)
		//return debug;
	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	static WCHAR buffer[12];
	LPCWSTR path;

	switch (measure->type)
	{

	case MEASURE_TITLE:
		return g_KwData->GetTitle();

	case MEASURE_ARTIST:
		return g_KwData->GetArtist();

	case MEASURE_TITLEARTIST:
		return g_KwData->GetTitleArtist();

	case MEASURE_PLAYERPATH:
		path = g_KwData->GetPlayerPath();
		path = !*path ? measure->playerpath.c_str() : path;
		return path;

	case MEASURE_STATUS:
		_itow_s(g_KwData->GetStatus(), buffer, 10);
		return buffer;
	}

	return nullptr;

}


PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;

	if (!_wcsicmp(args, L"Open"))
	{
		g_KwData->OpenPlayer(measure->playerpath);
	}
	else if (g_KwData->GetStatus())
	{
		if (!_wcsicmp(args, L"Close"))
		{
			g_KwData->ClosePlayer();
		}
		else if (!_wcsicmp(args, L"Restore"))
		{
			g_KwData->RestorePlayer();
		}
		else if (!_wcsicmp(args, L"Minimize"))
		{
			g_KwData->MinimizePlayer();
		}
		else if (!_wcsicmp(args, L"PlayPause"))
		{
			g_KwData->PlayPause();
		}
		else if (!_wcsicmp(args, L"Stop"))
		{
			g_KwData->Stop();
		}
		else if (!_wcsicmp(args, L"Previous"))
		{
			g_KwData->Previous();
		}
		else if (!_wcsicmp(args, L"Next"))
		{
			g_KwData->Next();
		}
		else if (!_wcsicmp(args, L"VolumeMute"))
		{
			g_KwData->VolumeMute();
		}
		else if (!_wcsicmp(args, L"VolumeUp"))
		{
			g_KwData->VolumeUp();
		}
		else if (!_wcsicmp(args, L"VolumeDown"))
		{
			g_KwData->VolumeDown();
		}
		else if (!_wcsicmp(args, L"Hide"))
		{
			g_KwData->HideToTray();
		}
		else if (!_wcsicmp(args, L"MiniMode"))
		{
			g_KwData->MiniMode();
		}
		else
		{
			RmLog(LOG_WARNING, L"KwMusic.dll: Unknown Command");
		}
	}
	else
	{
		RmLog(LOG_WARNING, L"KwMusic.dll: KwMusic.exe not found");
	}
}