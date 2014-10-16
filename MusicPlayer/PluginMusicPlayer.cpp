#include "PluginMusicPlayer.h"
#include "KwMusic.h"
#include "QQMusic.h"
#include "BaiduMusic.h"
#include "KgMusic.h"


vector<Measure*> g_Measures;
//bool g_Initialized = false;

//HANDLE g_UpdateThread = NULL;
//CRITICAL_SECTION g_CriticalSection;

/*#define UpdateValue 1000

unsigned __stdcall g_UpdateProc(void* pParam)
{
	bool KwChanged = false;
	bool KgChanged = false;
	bool QQChanged = false;
	bool BaiduChanged = false;
	Player* c_KwMusic;
	Player* c_KgMusic;
	Player* c_QQMusic;
	Player* c_BaiduMusic;

	do
	{
		EnterCriticalSection(&g_CriticalSection);

		//更新结构体指针
		c_KwMusic = KwMusic::c_Player;
		c_KgMusic = KgMusic::c_Player;
		c_QQMusic = QQMusic::c_Player;
		c_BaiduMusic = BaiduMusic::c_Player;

		//检查指针，更新播放器信息，检查TrackChanged
		if (c_KwMusic)
		{
			c_KwMusic->UpdateData();
			KwChanged = c_KwMusic->m_TrackChanged;
			c_KwMusic->m_TrackChanged = false;
		}
		if (c_KgMusic)
		{
			c_KgMusic->UpdateData();
			KgChanged = c_KgMusic->m_TrackChanged;
			c_KgMusic->m_TrackChanged = false;
		}
		if (c_QQMusic)
		{
			c_QQMusic->UpdateData();
			KwChanged = c_QQMusic->m_TrackChanged;
			c_QQMusic->m_TrackChanged = false;
		}
		if (c_BaiduMusic)
		{
			c_BaiduMusic->UpdateData();
			KwChanged = c_BaiduMusic->m_TrackChanged;
			c_BaiduMusic->m_TrackChanged = false;
		}
		
		//TrackChange动作
		if (KwChanged || KgChanged || QQChanged || BaiduChanged)
		{
			LPCWSTR Option;
			vector<Measure*>::iterator iter = g_Measures.begin();
			for (; iter < g_Measures.end(); iter++)
			{
				Option = (*iter)->trackchangeAction.c_str();
				if (*Option)
				{
					switch ((*iter)->name)
					{
					case PLAYER_KUWO:
						if (KwChanged)
							RmExecute(RmGetSkin((*iter)->rm), Option);
						break;
					case PLAYER_KUGOU:
						if (KgChanged)
							RmExecute(RmGetSkin((*iter)->rm), Option);
						break;
					case PLAYER_QQ:
						if (QQChanged)
							RmExecute(RmGetSkin((*iter)->rm), Option);
						break;
					case PLAYER_BAIDU:
						if (BaiduChanged)
							RmExecute(RmGetSkin((*iter)->rm), Option);
						break;
						
					}
				}
			}
			KwChanged = KgChanged = QQChanged = BaiduChanged = false;
		}

		LeaveCriticalSection(&g_CriticalSection);

		Sleep(UpdateValue);


	} while (true);

	return 0;
}
*/


PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
	measure->rm = rm;
		
	//记录Measure	
	g_Measures.push_back(measure);
			
	
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;

	//清除Measure记录
	vector<Measure*>::iterator iter = find(
		g_Measures.begin(),
		g_Measures.end(),
		measure);

	g_Measures.erase(iter);


	//结构体卸载
	measure->player->RemoveInstantce();

	delete measure;
}


PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	measure->rm = rm;
	
	//初始化 PlayerName
	LPCWSTR str = RmReadString(rm, L"PlayerName", L"");

	if (!_wcsicmp(L"KwMusic", str))
	{
		measure->name = PLAYER_KUWO;
		measure->player = KwMusic::Create();
	}
	else if (!_wcsicmp(L"KgMusic", str))
	{
		measure->name = PLAYER_KUGOU;
		measure->player = KgMusic::Create();
	}
	else if (!_wcsicmp(L"QQMusic", str))
	{
		measure->name = PLAYER_QQ;
		measure->player = QQMusic::Create();
	}
	else if (!_wcsicmp(L"BaiduMusic", str))
	{
		measure->name = PLAYER_BAIDU;
		measure->player = BaiduMusic::Create();
	}
	else
	{

		measure->name = PLAYER_KUWO;
		measure->player = KwMusic::Create();

		wstring error = L"MusicPlayer.dll: Invalid PlayerName=";
		error += str;
		error += L" in [";
		error += RmGetMeasureName(rm);
		error += L"]";
		RmLog(LOG_WARNING, error.c_str());
	}



	//初始化 PlayerType
	str = RmReadString(rm, L"PlayerType", L"");

	if (_wcsicmp(L"TITLE", str) == 0)
	{
		measure->type = MEASURE_TITLE;
	}
	else if (_wcsicmp(L"ARTIST", str) == 0)
	{
		measure->type = MEASURE_ARTIST;
	}
	else if (_wcsicmp(L"TRACK", str) == 0)
	{
		measure->type = MEASURE_TRACK;
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
	else if (_wcsicmp(L"COVER", str) == 0)
	{
		measure->type = MEASURE_COVER;
		switch (measure->name)
		{
		case PLAYER_KUGOU:
		case PLAYER_QQ:
			measure->player->m_RequireCover = true;
		}
	}
	else
	{
		std::wstring error = L"MusicPlayer.dll: Invalid PlayerType=";
		error += str;
		error += L" in [";
		error += RmGetMeasureName(rm);
		error += L"]";
		RmLog(LOG_WARNING, error.c_str());
	}

	measure->trackChangeAction = RmReadString(rm, L"TrackChangeAction", L"");

}

PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	Player* player = measure->player;

	player->UpdateData();

	if (player->m_TrackChanged && !measure->trackChangeAction.empty())
		RmExecute(RmGetSkin(measure->rm), measure->trackChangeAction.c_str());

	if (measure->type == MEASURE_STATUS)
		return player->GetStatus();
	
	return 0.0;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	Player* player = measure->player;
	static WCHAR buffer[12];
	LPCWSTR str;

	switch (measure->type)
	{
	case MEASURE_TITLE:
		return player->GetTitle();
		break;

	case MEASURE_ARTIST:
		return player->GetArtist();
		break;

	case MEASURE_TRACK:
		return player->GetTrack();
		break;

	case MEASURE_PLAYERPATH:
		str = player->GetPlayerPath();
		return !*str ? measure->playerpath.c_str() : str;
		break;

	case MEASURE_STATUS:
		_itow_s(player->GetStatus(), buffer, 10);
		return buffer;
		break;

	case MEASURE_COVER:
		return player->GetCoverPath();
		break;
	}


	return nullptr;

}


PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	Player* player = measure->player;
	
	if (!_wcsnicmp(args, L"Open",4))
	{
		player->OpenPlayer(measure->playerpath);
	}
	else if (player->GetStatus())
	{
		if (!_wcsnicmp(args, L"Close",5))
		{
			player->ClosePlayer();
		}
		else if (!_wcsicmp(args, L"Restore"))
		{
			player->RestorePlayer();
		}
		else if (!_wcsicmp(args, L"Minimize"))
		{
			player->MinimizePlayer();
		}
		else if (!_wcsicmp(args, L"PlayPause"))
		{
			player->PlayPause();
		}
		else if (!_wcsicmp(args, L"Stop"))
		{
			player->Stop();
		}
		else if (!_wcsicmp(args, L"Previous"))
		{
			player->Previous();
		}
		else if (!_wcsicmp(args, L"Next"))
		{
			player->Next();
		}
		else if (!_wcsicmp(args, L"VolumeMute"))
		{
			player->VolumeMute();
		}
		else if (!_wcsicmp(args, L"VolumeUp"))
		{
			player->VolumeUp();
		}
		else if (!_wcsicmp(args, L"VolumeDown"))
		{
			player->VolumeDown();
		}
		else if (!_wcsnicmp(args, L"Hide",4))
		{
			player->HideToTray();
		}
		else if (!_wcsicmp(args, L"MiniMode"))
		{
			player->MiniMode();
		}
		else
		{
			player->ExecuteBang(args);
		}
	}
	else
	{
		RmLog(LOG_WARNING, L"MusicPlayer.dll: Player isn't running");
	}
}





/*
**发送按键消息
*/
//向全局发送单一键击
void SendKey(WORD key)
{
	KEYBDINPUT kbi = { 0 };
	kbi.wVk = key;
	kbi.dwExtraInfo = (ULONG_PTR)GetMessageExtraInfo();

	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki = kbi;

	SendInput(1, &input, sizeof(INPUT));
}
//向窗口发送单一键击
void SendKey(HWND hWnd, UINT key)
{
	UINT VSC = MapVirtualKey(key, 0);
	PostMessage(hWnd, WM_KEYDOWN, key, 0x00000001 | VSC << 16);
	PostMessage(hWnd, WM_KEYUP, key, 0xC0000001 | VSC << 16);
}
//向窗口发送带Ctrl的键击
void SendKey(HWND hWnd, UINT key, bool ctrl)
{
	UINT VSC_CTRL = MapVirtualKey(VK_CONTROL, 0) << 16;
	UINT VSC = MapVirtualKey(key, 0) << 16;

	if (key >= 'A' && key <= 'Z')
	{	//字母

		PostMessage(hWnd, WM_KEYDOWN, VK_CONTROL, 0x00000001 | VSC_CTRL);
		PostMessage(hWnd, WM_KEYDOWN, key, 0x00000001 | VSC);
		//PostMessage(hWnd, WM_CHAR,		key - 0x40,	0x00000001 | VSC);
		PostMessage(hWnd, WM_KEYUP, key, 0xC0000001 | VSC);
		PostMessage(hWnd, WM_KEYUP, VK_CONTROL, 0xC0000001 | VSC_CTRL);
	}
	else
	{	//方向键
		PostMessage(hWnd, WM_KEYDOWN, VK_CONTROL, 0x00000001 | VSC_CTRL);
		PostMessage(hWnd, WM_KEYDOWN, key, 0x01000001 | VSC);
		PostMessage(hWnd, WM_KEYUP, key, 0xC1000001 | VSC);
		PostMessage(hWnd, WM_KEYUP, VK_CONTROL, 0xC0000001 | VSC_CTRL);
	}
}
//向全局发送带修饰键的键击
void SendKey(BYTE key, bool ctrl, bool shift, bool alt)
{
	if (ctrl)
		keybd_event(VK_CONTROL, 0, 0, 0);
	if (shift)
		keybd_event(VK_SHIFT, 0, 0, 0);
	if (alt)
		keybd_event(VK_MENU, 0, 0, 0);

	keybd_event(key, 0, 0, 0);
	keybd_event(key, 0, KEYEVENTF_KEYUP, 0);


	if (alt)
		keybd_event(VK_MENU, 0, KEYEVENTF_KEYUP, 0);
	if (shift)
		keybd_event(VK_SHIFT, 0, KEYEVENTF_KEYUP, 0);
	if (ctrl)
		keybd_event(VK_CONTROL, 0, KEYEVENTF_KEYUP, 0);

}