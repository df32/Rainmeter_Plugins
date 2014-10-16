#include <Windows.h>
#include <process.h>
#include <string>
#include <vector>
#include "../../API/RainmeterAPI.h"

using namespace std;

/*
[mUpdate]
Plugin=Update
Measures= m1 | m2 | m3
Meters= meter1
Update=50
StartOnLoad=1
RedrawSkin=1
UpdateSKin=0

*/

struct Measure
{
	Measure():
	rm(),
	m_Initialized(false),
	m_UpdateThread(),
	stopLooping(false),
	Update(100),
	SkinRedraw(true),
	SkinUpdate(false),
	counting(0.0)
	{}

	~Measure()
	{
		if (m_UpdateThread)
		{
			TerminateThread(m_UpdateThread, 0);
		}
	}

	bool m_Initialized;
	void* rm;

	void StartThread();
	HANDLE m_UpdateThread;
	static unsigned __stdcall UpdateThreadProc(void* pParam);

	bool stopLooping;
	int Update;
	bool SkinRedraw;
	bool SkinUpdate;

	vector<wstring> Meters;
	vector<wstring> Measures;
	vector<wstring> MeterGroups;
	vector<wstring> MeasureGroups;

	double counting;

};

void getTarget(void* rm, LPCWSTR option, vector<wstring>& can)
{
	can.clear();
	wstring data = RmReadString(rm, option, L"");
	if (data.empty())
		return;

	data += L" ";
	wstring::size_type pos;
	while (true)
	{
		pos = data.find_first_of(L" |");
		can.push_back(data.substr(0, pos));
		data.erase(0, pos);
		pos = data.find_first_not_of(L" |");
		if (pos == wstring::npos)
			break;
		data.erase(0, pos);
	}
}

void Measure::StartThread()
{
	if (m_UpdateThread)		//若线程未结束，则结束
	{
		TerminateThread(m_UpdateThread, 0);
	}
	
	getTarget(rm, L"Meters",		Meters);
	getTarget(rm, L"Measures",		Measures);
	getTarget(rm, L"MeterGroup",	MeterGroups);
	getTarget(rm, L"MeasureGroup",	MeasureGroups);
	
	
	Update = RmReadInt(rm, L"Update", 100);
	if (Update > 10000 || Update < 10)
	{
		RmLog(LOG_ERROR, L"Update.dll: Update value is out of range (10~10000 ms)");
		counting = -1.0;
		return;
	}

	int sr = RmReadInt(rm, L"RedrawSkin", 2);
	SkinRedraw = sr == 2 ?
		(Meters.empty() && MeterGroups.empty() ? false : true) :
		sr > 0;

	SkinUpdate = RmReadInt(rm, L"UpdateSkin", 0) > 0;

	stopLooping = false;
	counting = 0.0;

	unsigned int id;
	HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, UpdateThreadProc, this, 0, &id);		//创建新线程
	if (thread)
	{
		m_UpdateThread = thread;
	}
	else
	{
		RmLog(LOG_ERROR, L"Update.dll: Failed to start update thread");
		counting = -1.0;
	}
}

void updateTarget(void* rm, vector<wstring>& tar, LPCWSTR bang)
{
	if (tar.empty())
		return;

	wstring command;
	vector<wstring>::iterator iter = tar.begin();
	for (; iter != tar.end(); ++iter)
	{
		command = bang;
		command += L" ";
		command += *iter;
		RmExecute(RmGetSkin(rm), command.c_str());
	}
}

unsigned __stdcall Measure::UpdateThreadProc(void* pParam)
{
	Measure* measure = (Measure*)pParam;
	while (!measure->stopLooping)
	{
		measure->counting++;

		updateTarget(measure->rm, measure->Measures,			L"!UpdateMeasure");
		updateTarget(measure->rm, measure->MeasureGroups,	L"!UpdateMeasureGroup");
		updateTarget(measure->rm, measure->Meters,			L"!UpdateMeter");
		updateTarget(measure->rm, measure->MeterGroups,		L"!UpdateMeterGroup");
		
		if (measure->SkinRedraw)
			RmExecute(RmGetSkin(measure->rm), L"!Redraw");

		if (measure->SkinUpdate)
			RmExecute(RmGetSkin(measure->rm), L"!Update");

		if (!measure->stopLooping)
			Sleep(measure->Update);
	}
	return 0;
}


PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->rm = rm;
	if (!measure->m_Initialized)
	{
		measure->m_Initialized = true;
		if (RmReadInt(rm, L"StartOnLoad", 0) > 0)
		{
			measure->StartThread();
		}
	}
}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	return measure->counting;
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	if (!_wcsicmp(args, L"Start"))
	{
		measure->StartThread();
	}
	else if (!_wcsicmp(args, L"Stop"))
	{
		measure->stopLooping = true;
	}
	else if (!_wcsicmp(args, L"Terminate"))
	{
		if (measure->m_UpdateThread)
		{
			TerminateThread(measure->m_UpdateThread, 0);
		}
		measure->counting = 0.0;
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	delete measure;
}
