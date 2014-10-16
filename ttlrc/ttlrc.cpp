#include "StdAfx.h"
#include "Internet.h"
#include "Lyrics.h"
#include "StringUtil.h"


using namespace std;

//h

enum MeasureState {
	ms_ServerError = -4,		//服务器错误
	ms_NoMatch = -3,			//无匹配结果
	ms_EmptyResult = -2,		//空数据
	ms_Error = -1,
	ms_Standby = 0,
	ms_Downloading = 1,
	ms_SelectItem = 2
};

enum DownMode {
	dm_AutoDown = 0,
	dm_DownList,
	dm_DownItem
};


struct Measure
{
	Measure() :
	rm(),
	skin(),
	m_State(ms_Standby),
	m_Flag(dm_AutoDown),
	m_Initialized(false),
	m_InternetThread()
	{}

	~Measure()
	{
		if (m_InternetThread)
		{
			TerminateThread(m_InternetThread, 0);
		}
	}
	
	void* skin;
	void* rm;
	bool m_Initialized;

	HANDLE m_InternetThread;

	void FindLyrics();
	void SaveToLocal();
	static unsigned __stdcall LyricsThreadProc(void* pParam);
	

	wstring out;

	wstring m_Artist;
	wstring m_Title;
	wstring m_FinishAction;
	int m_Download = 0;
	int m_DownloadANSI = 0;
	wstring m_DownloadFileName;
	wstring m_DownloadFilePath;
	


	MeasureState m_State;

	DownMode m_Flag;

	vector<lrcItem> lrcItemList;
	lrcItem* CurrentItem;
	
};




//cpp

bool g_Initialized = false;
static int measureCount = 0;

//BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
//{
//	switch (fdwReason)
//	{
//	case DLL_PROCESS_ATTACH:
//		g_Instance = hinstDLL;
//
//		// Disable DLL_THREAD_ATTACH and DLL_THREAD_DETACH notification calls
//		DisableThreadLibraryCalls(hinstDLL);
//		break;
//	}
//
//	return TRUE;
//}

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;

	if (!g_Initialized)
	{
		Internet::Initialize();
		g_Initialized = true;
	}
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	
	if (--measureCount == 0)
		Internet::Finalize();

	delete measure;
}

PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->rm = rm;
	measure->skin = RmGetSkin(rm);
	if (!measure->m_Initialized)		//Measure首次加载
	{
		measureCount++;
		measure->m_Initialized = true;
		
		measure->m_FinishAction = RmReadString(rm, L"FinishAction", L"");
		measure->m_Download = RmReadInt(rm, L"Download", 0);

		if (measure->m_Download > 0)
		{
			measure->m_DownloadANSI = RmReadInt(rm, L"DownloadANSI", 0);

			if (measure->m_Download > 1)
				//(!PathIsDirectory(measure->m_DownloadFilePath.c_str()))
			{
				measure->m_DownloadFileName = RmReadString(rm, L"DownloadFileName", L"%A - %T");
				measure->m_DownloadFilePath = RmReadPath(rm, L"DownloadFilePath", L"");

			}
			else
			{
				WCHAR buffer[MAX_PATH];
				GetTempPath(MAX_PATH, buffer);
				measure->m_DownloadFilePath = buffer;
				
				memset(buffer, 0, MAX_PATH);
				GetTempFileName(0, L"lrc", 0, buffer);
				measure->m_DownloadFileName;
			}
			
		}
	}
}


PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	return (double)measure->m_State;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	return measure->out.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	measure->m_Artist = RmReadString(measure->rm, L"Artist", L"");
	measure->m_Title = RmReadString(measure->rm, L"Title", L"");

	if (!_wcsicmp(args, L"DownLoad"))		//命令自动下载歌词
	{
		measure->m_Flag = dm_AutoDown;

		if (measure->m_Artist.empty() && measure->m_Title.empty())	//若标题、艺术家同时为空，则略过
		{
			measure->m_State = ms_Error;
			RmLog(LOG_DEBUG, L"ttLrc.dll: Both artist and title are blank");
		}
		else
		{
			measure->FindLyrics();
		}
	}
	else if(!_wcsicmp(args, L"ForceDownLoad"))		//命令自动下载歌词
	{
		measure->m_Flag = dm_AutoDown;

		if (measure->m_Artist.empty() && measure->m_Title.empty())	//若标题、艺术家同时为空，则略过
		{
			measure->m_State = ms_Error;
			RmLog(LOG_DEBUG, L"ttLrc.dll: Both artist and title are blank");
		}
		else
		{
			if (measure->m_InternetThread)
			{
				TerminateThread(measure->m_InternetThread, 0);
				measure->m_InternetThread = nullptr;
			}
			measure->FindLyrics();
		}
	}
	else if (!_wcsicmp(args, L"DownList"))	//命令下载歌词列表并等待选择
	{
		measure->m_Flag = dm_DownList;

		if (measure->m_Artist.empty() && measure->m_Title.empty())	//若标题、艺术家同时为空，则略过
		{
			measure->m_State = ms_Error;
			RmLog(LOG_DEBUG, L"ttLrc.dll: Both artist and title are blank");
		}
		else
		{
			measure->FindLyrics();
		}
	}
	else if (!_wcsnicmp(args, L"DownItem", 8))	//命令选择歌词项并下载
	{
		measure->m_Flag = dm_DownItem;

		if (measure->lrcItemList.empty())
		{
			measure->m_State = ms_Error;
			RmLog(LOG_DEBUG, L"ttLrc.dll: Empty lyric list");
			return;
		}

		LPCWSTR arg = wcschr(args, L' ');
		lrcItem* item = nullptr;
		if (!arg)
		{
			item = &measure->lrcItemList[0];
		}
		else
		{
			arg++;
			//匹配ID
			vector <lrcItem>::iterator iter = measure->lrcItemList.begin();
			for (; iter != measure->lrcItemList.end(); ++iter)
			{
				if (iter->id == arg)
				{
					item = &(*iter);
					break;
				}
			}
		}
		if (item == nullptr)
		{
			measure->m_State = ms_Error;
			RmLog(LOG_DEBUG, L"ttLrc.dll: Mismatched ID");
		}
		else
		{
			measure->CurrentItem = item;
			measure->FindLyrics();
		}

	}
	else if (!_wcsicmp(args, L"Stop"))		//命令停止下载线程
	{
		TerminateThread(measure->m_InternetThread, 0);
		//CloseHandle(measure->m_InternetThread);
		measure->m_InternetThread = nullptr;
		measure->m_State = ms_Standby;
	}
}




//palyer.cpp
/*
** Default implementation for getting lyrics.
**
*/
void Measure::FindLyrics()
{
	if (!m_InternetThread)		//若线程未结束，则跳过
	{
		out.clear();

		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, LyricsThreadProc, this, 0, &id);		//创建新线程
		if (thread)
		{
			m_State = ms_Downloading;
			m_InternetThread = thread;
		}
		else
		{
			m_State = ms_Error;
			RmLog(LOG_ERROR, L"ttLrc.dll: Failed to start download thread");
		}
	}
	else
	{
		m_State = ms_Error;
		RmLog(LOG_ERROR, L"ttLrc.dll: Previous download thread isn't finished");
	}
}

void Measure::SaveToLocal()
{
	if (!m_Download) return;

	wstring path;
	
	if (m_Download == 1)
	{
		path = m_DownloadFilePath;
		path += L"\\";
		path += m_DownloadFileName;
	}
	else if (m_Download == 2)
	{
		path = m_DownloadFilePath;
		wstring filename = m_DownloadFileName;
		wstring::size_type pos = filename.find(L"%A");
		if (pos != wstring::npos)
		{
			filename.erase(pos, 2);
			filename.insert(pos, m_Artist);
		}
		
		pos = filename.find(L"%T");
		if (pos != wstring::npos)
		{
			filename.erase(pos, 2);
			filename.insert(pos, m_Title);
		}

		WCHAR* InvalidChar = L"\\/:*?\"<>|";
		for (pos = 0; pos < filename.length(); pos++)
		{
			if (wcschr(InvalidChar, filename[pos]))
			{
				filename.erase(pos);
				pos--;
			}
		}

		if (filename.empty())
			filename = L"0";

		path += L"\\";
		path += filename;
		path += L".lrc";
	}
	else
		return;

	FILE* file;
	if (_wfopen_s(&file, path.c_str(), L"wb"))
		return RmLog(LOG_ERROR, L"ttLrc.dll: Failed to write file");

	if (!m_DownloadANSI)
	{
		WCHAR bom = 0xFFFE;
		fwrite(&bom, sizeof(WCHAR), 1, file);
		fwrite(out.c_str(), sizeof(WCHAR), out.length() + 1, file);
	}
	else
	{
		string strBuffer = StringUtil::Narrow(out, CP_ACP);
		fwrite(strBuffer.c_str(), sizeof(char), strBuffer.size(), file);
	}
	fclose(file);

	out = path;
}


/*
** Thread to download lyrics.
**
*/
unsigned __stdcall Measure::LyricsThreadProc(void* pParam)
{
	Measure* player = (Measure*)pParam;
	player->out.clear();

	wstring outcome;
	int ret;

	if (player->m_Flag == dm_AutoDown)
	{
		ret = Lyrics::GetFromInternet(player->m_Artist, player->m_Title, outcome);
	}		
	else if (player->m_Flag == dm_DownList)
	{
		ret = Lyrics::GetLrcList(player->m_Artist, player->m_Title, outcome, player->lrcItemList);
	}
		
	else if (player->m_Flag == dm_DownItem)
	{
		ret = Lyrics::GetLrcItem(player->CurrentItem, outcome);
	}
		
		
	if (!ret)
	{
		player->m_State = ms_Standby;
		player->out = outcome;

		if (player->m_Flag == dm_AutoDown || player->m_Flag == dm_DownItem)
			player->SaveToLocal();

	}
	else
	{
		player->m_State = (MeasureState) - ret;
		RmLog(LOG_DEBUG, outcome.c_str());
		RmLog(LOG_WARNING, L"ttLrc.dll: Failed to download");
	}

	CloseHandle(player->m_InternetThread);
	player->m_InternetThread = nullptr;

	if (!player->m_FinishAction.empty())
		RmExecute(player->skin, player->m_FinishAction.c_str());

	return 0;
}
