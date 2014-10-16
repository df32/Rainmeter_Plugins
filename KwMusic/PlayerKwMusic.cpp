#include "stdhead.h"
#include "PlayerKwMusic.h"


extern vector<Measure*> g_Measures;
//extern int g_mCount;
PlayerKwMusic* g_KwData;
//double debug = 0;

#define UpdateValue 500

/*
**返回信息
*/
LPCWSTR PlayerKwMusic::GetTitle()
	{
		return Title.c_str();
	}

LPCWSTR PlayerKwMusic::GetArtist()
	{
		return Artist.c_str();
	}

LPCWSTR PlayerKwMusic::GetTitleArtist()
	{
		return Track.c_str();
	}

LPCWSTR PlayerKwMusic::GetPlayerPath()
	{
		return PlayerPath.c_str();
	}

int PlayerKwMusic::GetStatus()
	{
		return Handle ? 1 : 0;
	}



/*
**窗口操作
*/
void PlayerKwMusic::OpenPlayer(wstring& path)
{
	if (!IsWindow(Handle))
	{
		if (path.empty() && PlayerPath.empty())
		{
			HKEY hKey;
			RegOpenKeyEx(HKEY_CLASSES_ROOT,
				L"kuwo\\Shell\\open\\command",
				0,
				KEY_QUERY_VALUE,
				&hKey);

			DWORD size = 512;
			WCHAR* data = new WCHAR[size];
			DWORD type = 0;

			if (RegQueryValueEx(hKey,
				nullptr,
				nullptr,
				(LPDWORD)&type,
				(LPBYTE)data,
				(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					path = data;
					path.erase(0, 1);				// Get rid of the leading quote
					path.resize(path.length() - 6);	// And the " "%1" at the end
					ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
					PlayerPath = path;
				}
				else
				{
					ShellExecute(nullptr, L"open", L"KwMusic.exe", nullptr, nullptr, SW_SHOW);
				}
			}
			else
			{
				ShellExecute(nullptr, L"open", L"KwMusic.exe", nullptr, nullptr, SW_SHOW);
			}

			delete[] data;
			RegCloseKey(hKey);
		}
		else
		{
			path = path.empty() ? PlayerPath : path;
			ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
		}
	}
	else
	{
		ShowWindow(Handle, SW_SHOWNORMAL);
		BringWindowToTop(Handle);
	}

}

void PlayerKwMusic::ClosePlayer()
{
	if (IsWindow(Handle))
		PostMessage(Handle, WM_CLOSE, 0, 0);
}

void PlayerKwMusic::RestorePlayer()
{
	if (IsWindow(Handle))
	{
		PostMessage(Handle, WM_SYSCOMMAND, SC_RESTORE, 0);
		BringWindowToTop(Handle);
	}

}

void PlayerKwMusic::MinimizePlayer()
{
	if (IsWindow(Handle))
		PostMessage(Handle, WM_SYSCOMMAND, SC_MINIMIZE, 0);
}



/*
**按键操作
*/
void PlayerKwMusic::PlayPause()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_F5);
}

void PlayerKwMusic::Stop()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_F6);
}

void PlayerKwMusic::Previous()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_LEFT, 1);
}

void PlayerKwMusic::Next()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_RIGHT, 1);
}

void PlayerKwMusic::VolumeMute()
{
	if (IsWindow(Handle))
		PressKey(Handle, 'S', 1);
}

void PlayerKwMusic::VolumeUp()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_UP, 1);
}

void PlayerKwMusic::VolumeDown()
{
	if (IsWindow(Handle))
		PressKey(Handle, VK_DOWN, 1);
}

void PlayerKwMusic::HideToTray()
{
	if (IsWindow(Handle))
		PressKey(Handle, 'H', 1);
}

void PlayerKwMusic::MiniMode()
{
	if (IsWindow(Handle))
		PressKey(Handle, 'M', 1);
}


/*
**
*/
void PlayerKwMusic::Initialize()
{
	//获取窗口句柄?
	Handle = FindWindowEx(0, 0, L"kwmusicmaindlg", 0);

	//获取PlayerPath
	if (Handle && PlayerPath.empty())
	{
		DWORD dwProcessId;

		GetWindowThreadProcessId(Handle, &dwProcessId);
		HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);

		WCHAR sProcPath[MAX_PATH];
		if (GetModuleFileNameEx(hProcess, NULL, sProcPath, MAX_PATH))
			PlayerPath = sProcPath;
		else
			PlayerPath = L"";

		CloseHandle(hProcess);
	}

	//创建线程
	if (!UpdateThread)
	{
		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, UpdateProc, this, 0, &id);		//创建新线程
		if (thread)
		{
			UpdateThread = thread;
		}
		else
		{
			RmLog(LOG_ERROR, L"KwMusic.dll: Failed to create update thread");
		}
	}
}


unsigned __stdcall PlayerKwMusic::UpdateProc(void* pParam)
{
	PlayerKwMusic* KwData = (PlayerKwMusic*)pParam;
	DWORD time = GetTickCount();

	do
	{
		wstring LastTrack = KwData->Track;
		bool abnormal = false;

		if (!IsWindow(KwData->Handle))
		{
			KwData->Handle = NULL;

			DWORD newtime = GetTickCount();
			if (newtime - time > 5000)
			{
				time = newtime;
				KwData->Initialize();
			}
		}

		if (!KwData->Handle)
			abnormal = true;

		if (!abnormal)
		{
			WCHAR Buffer[102];
			Buffer[100] = '\0';
			GetWindowText(KwData->Handle, Buffer, 100);
			//L"With You-AAA-酷我音乐 "
			//L"子-酷我音乐 虹の約束-小松未可"

			wstring data = Buffer;
			data += data;
			data += data;
			
			wstring::size_type pos;
			pos = data.find(L"-酷我音乐 ");
			
			if (pos == wstring::npos)
				abnormal = true;
			else
			{
				pos += 6;
				data.erase(0, pos);
				pos = data.find(L"-酷我音乐 ");
				data.resize(pos);
			}
			

			if (!abnormal && LastTrack.compare(data))
			{
				pos = 0;
				int pos2 = 0, pos3 = 0;
				int len = data.length();
				for (int i = 1; i < len; i++)
				{
					if (data[i] == L'-')
					{
						if (data[i - 1] == L' ')
						{
							if (data[i + 1] == L' ')
								pos3 = i;
							else
								pos2 = i;
						}
						else
						{
							if (data[i + 1] == L' ')
								pos2 = i;
							else
							{
								pos = i;
								break;		//真正的分隔符左右两侧应当都不为空格
							}

						}
					}
				}
				pos = pos == 0 ? pos2 : pos;
				pos = pos == 0 ? pos3 : pos;

				if (pos == 0)
					abnormal = true;
				else
				{
					KwData->Track.assign(data);
					KwData->Title.assign(data, 0, pos);
					KwData->Artist.assign(data.erase(0, pos + 1));
					abnormal = false;
				}
				
			}
		}
		
		if (abnormal)
		{
			KwData->Track.clear();
			KwData->Title.clear();
			KwData->Artist.clear();
		}
		

		if (KwData->Track.compare(LastTrack))
			KwData->TrackChange();

		Sleep(UpdateValue);

	} while (true);
}


void PlayerKwMusic::TrackChange()
{
	LPCWSTR Option;
	
	vector<Measure*>::iterator iter = g_Measures.begin();
	for (; iter != g_Measures.end(); iter++)
	{
		Option = RmReadString((*iter)->rm, L"TrackChangeAction", nullptr);
		if (Option != nullptr)
			RmExecute(RmGetSkin((*iter)->rm), Option);
	}
}

