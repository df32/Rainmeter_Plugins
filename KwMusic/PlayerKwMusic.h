#ifndef __PLAYERKWMUSIC_H__
#define __PLAYERKWMUSIC_H__

#include "stdhead.h";


struct PlayerKwMusic
{

	wstring Track;
	wstring Title;
	wstring Artist;
	wstring PlayerPath;

	HWND Handle = NULL;
	HANDLE UpdateThread = NULL;


	void Initialize();
	void TrackChange();
	static unsigned __stdcall UpdateProc(void* pParam);

	//返回信息
	LPCWSTR GetTitle();
	LPCWSTR GetArtist();
	LPCWSTR GetTitleArtist();
	LPCWSTR GetPlayerPath();
	int GetStatus();

	//窗口操作
	void OpenPlayer(wstring& path);
	void ClosePlayer();
	void RestorePlayer();
	void MinimizePlayer();



	//按键操作
	void PressKey(HWND hWnd, UINT key)
	{
		UINT VSC = MapVirtualKey(key, 0);
		PostMessage(hWnd, WM_KEYDOWN, key, 0x00000001 | VSC << 16);
		PostMessage(hWnd, WM_KEYUP, key, 0xC0000001 | VSC << 16);
	}
	void PressKey(HWND hWnd, UINT key, bool ctrl)
	{
		UINT VSC_CTRL = MapVirtualKey(VK_CONTROL, 0) << 16;
		UINT VSC = MapVirtualKey(key, 0) << 16;

		if (key >= 'A' && key <= 'Z')
		{	//字母

			PostMessage(hWnd, WM_KEYDOWN,	VK_CONTROL,	0x00000001 | VSC_CTRL);
			PostMessage(hWnd, WM_KEYDOWN,	key,		0x00000001 | VSC);
			//PostMessage(hWnd, WM_CHAR,		key - 0x40,	0x00000001 | VSC);
			PostMessage(hWnd, WM_KEYUP,		key,		0xC0000001 | VSC);
			PostMessage(hWnd, WM_KEYUP,		VK_CONTROL,	0xC0000001 | VSC_CTRL);
		}
		else
		{	//方向键
			PostMessage(hWnd, WM_KEYDOWN,	VK_CONTROL,	0x00000001 | VSC_CTRL);
			PostMessage(hWnd, WM_KEYDOWN,	key,		0x01000001 | VSC);
			PostMessage(hWnd, WM_KEYUP,		key,		0xC1000001 | VSC);
			PostMessage(hWnd, WM_KEYUP,		VK_CONTROL,	0xC0000001 | VSC_CTRL);
		}
	}

	void PlayPause();
	void Stop();
	void Previous();
	void Next();
	void VolumeMute();
	void VolumeUp();
	void VolumeDown();
	void HideToTray();
	void MiniMode();

	~PlayerKwMusic()
	{
		if (UpdateThread)
		{
			TerminateThread(UpdateThread, 0);
		}
	}
};




#endif