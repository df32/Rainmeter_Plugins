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
#include "Internet.h"
#include "Lyrics.h"

using namespace std;
/*
** Download lyrics from TTPlayer
**
*/
int Lyrics::GetFromInternet(const std::wstring& artist, const std::wstring& title, std::wstring& out, std::vector<lrcItem>& Items)
{
	//RmLog(4, L"ttLrc.dll: GetFromInternet()");
	int ret = GetLrcList(artist, title, out, Items);

	if (ret)
		return ret;

	vector<lrcItem>::iterator iter = Items.begin();

	for (; iter != Items.end(); ++iter)
	{
		if (iter->artist.find(L"中日") != wstring::npos || iter->artist.find(L"中日") != wstring::npos)
		{
			break;
		}
	}

	if (iter != Items.end())
		return GetLrcItem(&(*iter), out);
	else
		return GetLrcItem(&Items[0], out);
}




/*
** 字符串转换为十六进制
** （第一次，移除\ 和\'使用UTF-16，二次使用utf-8）
*/
wstring SetToHex(const wstring& wstr)
{

	wchar_t wstr_b[122];
	wstr_b[120] = 0;
	wstr_b[121] = 0;
	wcscpy_s(wstr_b, 120, wstr.c_str());
	_wcslwr_s(wstr_b, wcslen(wstr_b) + 1);		//小写

	//wstr.assign(wstr_b);

	wchar_t * ignore_chars = L" '";
	wstring ret;
	union{
		wchar_t w;
		unsigned char  c[sizeof(wchar_t)];
	}un;

	for (int i = 0, max = wcslen(wstr_b); i < max; i++)
	{
		if (!wcschr(ignore_chars, wstr_b[i]))
		{
			wchar_t b[6];
			//unsigned char* c = (unsigned char*)&wstr_b[i];
			un.w = wstr_b[i];
			_snwprintf_s(b, 6, L"%.2X%.2X", un.c[0], un.c[1]);

			ret.append(b);
		}
	}

	return ret;
}


long Conv(int i) {
	long r = i % 4294967296;
	if (i >= 0 && r > 2147483648)
		r = r - 4294967296;

	if (i < 0 && r < 2147483648)
		r = r + 4294967296;
	return r;
}

wstring CreateCode(std::wstring& Id, std::wstring& artist, std::wstring& title)
{       //输入<歌手>, <标题>和lrcId

	string utf8str(Internet::NarrowUTF8(artist + title));	//转为utf-8	

	int length = utf8str.length();
	int lrcId = _wtoi(Id.c_str());
	int* song = new int[length];

	//将字节转换为数字
	for (int i = 0; i < length; i++)
	{
		song[i] = (int)utf8str[i];
	}


	int t1 = 0, t2 = 0, t3 = 0;
	t1 = (lrcId & 0x0000FF00) >> 8;
	if ((lrcId & 0x00FF0000) == 0) {
		t3 = 0x000000FF & ~t1;
	}
	else {
		t3 = 0x000000FF & ((lrcId & 0x00FF0000) >> 16);
	}

	t3 = t3 | ((0x000000FF & lrcId) << 8);
	t3 = t3 << 8;
	t3 = t3 | (0x000000FF & t1);
	t3 = t3 << 8;
	if ((lrcId & 0xFF000000) == 0) {
		t3 = t3 | (0x000000FF & (~lrcId));
	}
	else {
		t3 = t3 | (0x000000FF & (lrcId >> 24));
	}

	int j = length - 1;
	while (j >= 0) {
		int c = song[j];
		if (c >= 0x80) c = c - 0x100;

		t1 = (int)((c + t2) & 0x00000000FFFFFFFF);
		t2 = (int)((t2 << (j % 2 + 4)) & 0x00000000FFFFFFFF);
		t2 = (int)((t1 + t2) & 0x00000000FFFFFFFF);
		j -= 1;
	}
	j = 0;
	t1 = 0;
	while (j <= length - 1) {
		int c = song[j];
		if (c >= 128) c = c - 256;
		int t4 = (int)((c + t1) & 0x00000000FFFFFFFF);
		t1 = (int)((t1 << (j % 2 + 3)) & 0x00000000FFFFFFFF);
		t1 = (int)((t1 + t4) & 0x00000000FFFFFFFF);
		j += 1;
	}

	int t5 = (int)Conv(t2 ^ t3);
	t5 = (int)Conv(t5 + (t1 | lrcId));
	t5 = (int)Conv(t5 * (t1 | t3));
	t5 = (int)Conv(t5 * (t2 ^ lrcId));

	long t6 = (long)t5;
	if (t6 > 2147483648)
		t5 = (int)(t6 - 4294967296);

	delete[] song;

	wchar_t b[15];
	_itow_s(t5, b, 15, 10);
	wstring ret = b;
	return ret;		//

}

int Lyrics::GetLrcList(const std::wstring& ar, const std::wstring& ti, std::wstring& out, std::vector<lrcItem>& Items)
{
	wstring artist = ar;
	wstring title = ti;

	wstring url = L"http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?sh?Artist={0}&Title={1}&Flags=0";
	//<server name="歌词服务器(电信)" url="http://ttlrcct2.qianqian.com/dll/lyricsvr.dll"/>
	//<server name="歌词服务器(网通)" url="http://ttlrccnc.qianqian.com/dll/lyricsvr.dll"/>
	
	//查找替换{1}
	wstring::size_type pos;
	pos = url.find(L"{0}");
	url.replace(pos, 3, SetToHex(artist));
	pos = url.find(L"{1}");
	url.replace(pos, 3, SetToHex(title));

	//RmLog(4, L"ttLrc.dll: GetLrcList()");
	RmLog(4, url.c_str());
	Items.clear();

	wstring data = Internet::DownloadUrl(url, CP_UTF8);

	if (data.empty())
	{
		out = L"ttLrc.dll: 未能连接网络！";
		return 2;
	}
	else if (data.find(L"id=") == wstring::npos)
	{
		//<result errmsg="歌词服务正在重启" errcode="32007"></result>
		pos = data.find(L"errmsg=");
		if (pos != wstring::npos)
		{
			data.erase(0, pos + 8);
			pos = data.find_first_of(L'\"');
			out.assign(data, 0, pos);
			out = L"ttLrc.dll: " + out;
			return 4;
		}

		out = L"ttLrc.dll: 无匹配歌词！\n" + data;
		return 3;
	}
		
	Internet::DecodeReferences(data);
	out = data;
	
	// 获取了歌词列表
	// <lrc id="978794" artist="Barbarian On The Groove feat.片霧烈火" title="Nano Universe"></lrc>
	while (data.find(L"id=") != wstring::npos)
	{
		lrcItem Item;
		pos = data.find(L"id=") + 4;
		data.erase(0, pos);
		pos = data.find_first_of(L'\"');
		Item.id.assign(data, 0, pos);

		pos = data.find(L"artist=") + 8;
		data.erase(0, pos);
		pos = data.find_first_of(L'\"');
		Item.artist.assign(data, 0, pos);

		pos = data.find(L"title=") + 7;
		data.erase(0, pos);
		pos = data.find_first_of(L'\"');
		Item.title.assign(data, 0, pos);

		Items.push_back(Item);
	}
	return 0;
}


int Lyrics::GetLrcItem(const lrcItem* Item, std::wstring& out)
{
	wstring id = Item->id;
	wstring ar = Item->artist;
	wstring ti = Item->title;

	//计算Code
	wstring Code = CreateCode(id, ar, ti);

	wstring url = L"http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?dl?Id={0}&Code={1}";

	wstring::size_type pos;
	pos = url.find(L"{0}");
	url.replace(pos, 3, id);
	pos = url.find(L"{1}");
	url.replace(pos, 3, Code);

	//RmLog(4, L"ttLrc.dll: GetLrcItem()");
	RmLog(4, url.c_str());
	// 下载歌词页
	out = Internet::DownloadUrl(url, CP_UTF8);

	if (out.empty())
	{
		out = L"ttLrc.dll: 未能连接网络！";
		return 2;
	}
	else if (out.find(L"errmsg=") != wstring::npos)
	{
		pos = out.find(L"errmsg=") + 8;
		out.erase(0, pos);
		pos = out.find_first_of(L'\"');
		out.assign(out, 0, pos);
		out = L"ttLrc.dll: " + out;
		return 4;
	}
	return 0;
}










//
//
//
//
//bool Lyrics::GetFromTTPlayer(const std::wstring& ar, const std::wstring& ti, std::wstring& data)
//{
//	bool ret = false;
//	wstring artist = ar;
//	wstring title = ti;
//	wstring ID;
//	wstring url = L"http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?sh?Artist={0}&Title={1}&Flags=0";
//	//<server name="歌词服务器(电信)" url="http://ttlrcct2.qianqian.com/dll/lyricsvr.dll"/>
//	//<server name="歌词服务器(网通)" url="http://ttlrccnc.qianqian.com/dll/lyricsvr.dll"/>
//
//
//
//	//查找替换{1}
//	wstring::size_type pos;
//	pos = url.find(L"{0}");
//	url.replace(pos, 3, SetToHex(artist));
//	pos = url.find(L"{1}");
//	url.replace(pos, 3, SetToHex(title));
//
//	
//
//
//	RmLog(4, url.c_str());
//
//	//获取歌词列表网页
//	data = Internet::DownloadUrl(url, CP_UTF8);
//	if (!data.empty())
//	{
//		// 获取了歌词列表
//		// <lrc id="978794" artist="Barbarian On The Groove feat.片霧烈火" title="Nano Universe"></lrc>
//		if (pos != wstring::npos)
//		{
//			//wstring::size_type pos2;
//
//			pos = data.find(L"中日");		//优先匹配中日双语歌词
//			if (pos != wstring::npos)
//			{
//				pos = data.rfind(L"id=",pos) + 4;
//			}
//			else
//			{
//				pos = data.find(L"id=\"") + 4;
//			}
//			
//			data.erase(0, pos);
//			pos = data.find_first_of(L'\"') ;
//			ID.assign(data, 0, pos);
//
//			pos = data.find(L"artist=") + 8;
//			data.erase(0, pos);
//			pos = data.find_first_of(L'\"') ;
//			artist.assign(data, 0, pos);
//
//			pos = data.find(L"title=") + 7;
//			data.erase(0, pos);
//			pos = data.find_first_of(L'\"') ;
//			title.assign(data, 0, pos);
//			
//			//wstring msg = L"id=" + ID + L" ar=" + artist + L" ti=" + title;
//			
//			//RmLog(4, msg.c_str());
//
//			//计算Code
//			wstring Code = CreateCode(ID, artist, title);
//
//			url = L"http://ttlrcct2.qianqian.com/dll/lyricsvr.dll?dl?Id={0}&Code={1}";
//
//			pos = url.find(L"{0}");
//			url.replace(pos, 3, ID);
//			pos = url.find(L"{1}");
//			url.replace(pos, 3, Code);
//
//			//RmLog(4, url.c_str());
//			// 下载歌词页
//			data = Internet::DownloadUrl(url, CP_UTF8);
//			if (!data.empty())
//			{
//				ret = true;
//			}
//		}
//	}
//	return ret;
//}
//
