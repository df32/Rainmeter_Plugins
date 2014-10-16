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

#ifndef __LYRICS_H__
#define __LYRICS_H__


struct lrcItem
{
	std::wstring id;
	std::wstring artist;
	std::wstring title;
};


class Lyrics
{
public:
	static int GetFromInternet(const std::wstring& artist, const std::wstring& title, std::wstring& out)
	{
		std::vector<lrcItem> Items;
		int ret = GetFromInternet(artist, title, out, Items);
		Items.clear();
		return ret;
	}
	static int GetFromInternet(const std::wstring& artist, const std::wstring& title, std::wstring& out, std::vector<lrcItem>& Items);

	//static bool GetTTLrc(const std::wstring& ar, const std::wstring& ti, std::wstring& data, std::vector<lrcItem*>& Items);
	
	
	static int GetLrcList(const std::wstring& ar, const std::wstring& ti, std::wstring& out, std::vector<lrcItem>& Items);
	static int GetLrcItem(const lrcItem* Item, std::wstring& out);

private:
	
	
	//static bool GetFromTTPlayer(const std::wstring& artist, const std::wstring& title, std::wstring& data);
	

};


#endif