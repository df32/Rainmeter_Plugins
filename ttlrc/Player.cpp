/*
  Copyright (C) 2011 Birunthan Mohanathas (www.poiru.net)

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "StdAfx.h"
#include "Player.h"

/*
** Constructor.
**
*/
Player::Player() :
	m_Initialized(false),
	m_Measures(),
	m_InternetThread()
{}

/*
** Destructor.
**
*/
Player::~Player()
{

	if (m_InternetThread)
	{
		TerminateThread(m_InternetThread, 0);
	}
}

/*
** Default implementation for getting lyrics.
**
*/
void Player::FindLyrics()
{
	if (!m_InternetThread)		//若线程未结束，则跳过
	{
		m_Lyrics.clear();

		unsigned int id;
		HANDLE thread = (HANDLE)_beginthreadex(nullptr, 0, LyricsThreadProc, this, 0, &id);		//创建新线程
		if (thread)
		{
			m_InternetThread = thread;
		}
		else
		{
			RmLog(LOG_DEBUG, L"NowPlaying.dll: Failed to start lyrics thread");
		}
	}
}

/*
** Thread to download lyrics.
**
*/
unsigned __stdcall Player::LyricsThreadProc(void* pParam)
{
	Player* player = (Player*)pParam;

	std::wstring lyrics;

	if (Lyrics::GetFromInternet(player->m_Artist, player->m_Title, lyrics))
	{
		player->m_Lyrics = lyrics;
	}

	CloseHandle(player->m_InternetThread);
	player->m_InternetThread = nullptr;
	//插入FinishAction
	return 0;
}

/*
** Clear track information.
**
*/
void Player::ClearData(bool all)
{
	m_State = STATE_STOPPED;
	m_Artist.clear();
	m_Album.clear();
	m_Title.clear();
	m_Lyrics.clear();

	
}
