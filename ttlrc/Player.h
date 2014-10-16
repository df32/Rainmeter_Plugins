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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "Internet.h"
#include "Lyrics.h"



class __declspec(novtable) Player
{
public:
	Player();
	virtual ~Player() = 0;



	bool IsInitialized() const { return m_Initialized; }

	void FindLyrics();





protected:
	

	bool m_Initialized;
	

	INT m_Measures;

	std::wstring m_Artist;
	std::wstring m_Title;
	std::wstring m_Album;
	std::wstring m_Lyrics;
	

private:
	static unsigned __stdcall LyricsThreadProc(void* pParam);

	HANDLE m_InternetThread;
};




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