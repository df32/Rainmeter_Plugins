
#include <windows.h>
#include <string>
//#include <shlwapi.h>
#include "StringUtil.h"
#include "../../API/RainmeterAPI.h"
//#pragma comment(lib, "../../API/x32/Rainmeter.lib")

#define BUFFER_SIZE 8192

using namespace std;

struct Measure
{
	void* rm;
	bool Initialized;

	wstring pathname;
	int codepage;

	wstring ret;
	long size;
	int len;

	/*Measure() :
		Initialized(false),
		ret(L""),
		size(1),
		len(0)
	{}*/
};

PLUGIN_EXPORT void Initialize(void** data, void* rm)
{
	Measure* measure = new Measure;
	*data = measure;
	measure->rm = rm;
	measure->Initialized = false;
	measure->len = 0;
	measure->size = 1;
	//RmLog(LOG_DEBUG, L"Initialized!!");
}

void Read(void* data, void* rm)
{
	Measure* measure = (Measure*)data;

	measure->pathname = RmReadPath(rm, L"PathName", L"");
	int codepage = RmReadInt(rm, L"CodePage", -1);
	measure->codepage = codepage;
	measure->size = 1;
	measure->len = 0;
	
	//RmLog(LOG_DEBUG, L"ReadFile!!");

	if (!measure->pathname.empty())
	{

		FILE* file;

		if (!_wfopen_s(&file, measure->pathname.c_str(), L"r")) //文件存在
		{
			BYTE buffer[BUFFER_SIZE + 2];
			buffer[BUFFER_SIZE] = 0;
			buffer[BUFFER_SIZE + 1] = 0;
			fread(buffer, sizeof(wchar_t), 1, file);
			//预读取文件头，以检测BOM

			fseek(file, 0, SEEK_END);
			long size = ftell(file);				//文件总字节数
			fseek(file, 0, SEEK_SET);
			measure->size = size;

			if (size > 6)
			{
				//检测文件编码
				if (0xFEFE <= *(wchar_t*)buffer)
				{	//Unicode
					codepage = CP_WINUNICODE;

				}
				else if (codepage < 0 && (0xEFBB == *(wchar_t*)buffer) || 0xBBEF == *(wchar_t*)buffer)
				{	//UTF-8
					codepage = CP_UTF8;

				}
				else if (codepage < 0)
				{	//区分UTF-8与ANSI	(by myx87216)

					int i = 0;
					int goodbytes = 0;
					int asciibytes = 0;

					int size0 = fread(buffer, sizeof(BYTE), 1024, file) - 5;
					fseek(file, 0, SEEK_SET);
					if (size0 <= 0)
					{
						RmLog(2, L"ReadFile.dll: Unexpected error! size0<0 in #94");
						return;
					}

					//预读取文件1024字节，以识别utf编码
					do
					{
						if (buffer[i] >= 0 && buffer[i] <= 127)
						{	//ascii
							asciibytes++;
						}
						else if (buffer[i] >= 224 && buffer[i] <= 239
							&& buffer[i + 1] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 2] >= 128 && buffer[i + 2] <= 191)
						{
							goodbytes += 3;
							i += 2;
						}
						else if (buffer[i] >= 240 && buffer[i] <= 247
							&& buffer[i + 1] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 2] >= 128 && buffer[i + 2] <= 191
							&& buffer[i + 3] >= 128 && buffer[i + 3] <= 191)
						{
							goodbytes += 4;
							i += 3;
						}
						else if (buffer[i] >= 248 && buffer[i] <= 251
							&& buffer[i + 1] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 2] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 3] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 4] >= 128 && buffer[i + 4] <= 191)
						{
							goodbytes += 5;
							i += 4;
						}
						else if (buffer[i] >= 252 && buffer[i] <= 253
							&& buffer[i + 1] >= 128 && buffer[i + 1] <= 191
							&& buffer[i + 2] >= 128 && buffer[i + 2] <= 191
							&& buffer[i + 3] >= 128 && buffer[i + 3] <= 191
							&& buffer[i + 4] >= 128 && buffer[i + 4] <= 191
							&& buffer[i + 5] >= 128 && buffer[i + 5] <= 191)
						{
							goodbytes += 6;
							i += 5;
						}
						i++;
					} while (i < size0);
					if (asciibytes >= size0)
					{
						codepage = CP_ACP;
					}
					else
					{
						codepage = CP_ACP;
						if (100 * goodbytes / (size0 - asciibytes) > 95)
						{
							codepage = CP_UTF8;
						}
					}
				}

				//memset(buffer, 0, BUFFER_SIZE + 2);


				//读取文件，并编码转换
				if (codepage == CP_WINUNICODE)
				{	//unicode
					measure->ret.clear();

					do
					{
						fread(buffer, sizeof(char), BUFFER_SIZE, file);
						measure->ret += (wchar_t*)buffer;

					} while (ftell(file) < size);
					
					measure->len = ftell(file);
				}
				else
				{	//ansi or utf-8 or othercodepage
					codepage = (codepage <= 0 ? CP_ACP : codepage);
					string data;

					do
					{
						int len = fread(buffer, sizeof(char), BUFFER_SIZE, file);
						buffer[len] = 0;
						buffer[len + 1] = 0; 
						data += (char*)buffer;

					} while (ftell(file) < size);

					
					measure->len = ftell(file);
					measure->ret = StringUtil::Widen(data, codepage);
				}

			}
			else
			{
				measure->ret = L"empty file";
				measure->len = -1;
			}
			fclose(file);
		}
		else
		{
			measure->ret = L"invalid file";
			measure->len = -1;
			measure->size = 0;
		}
		RmExecute(RmGetSkin(rm), RmReadString(rm, L"FinishAction", L""));
		//当地址为空时不执行FinishAction
	}

}


PLUGIN_EXPORT void Reload(void* data, void* rm, double* maxValue)
{
	Measure* measure = (Measure*)data;
	
	 *maxValue = measure->size;

	//RmLog(LOG_DEBUG, L"Who are Reloading Me!!");

	if (!measure->Initialized)
	{
		measure->Initialized = true;
		Read(data, rm);
	}
}


PLUGIN_EXPORT double Update(void* data)
{
	Measure* measure = (Measure*)data;
	return (double)measure->len;
}

PLUGIN_EXPORT LPCWSTR GetString(void* data)
{
	Measure* measure = (Measure*)data;
	return measure->ret.c_str();
}

PLUGIN_EXPORT void ExecuteBang(void* data, LPCWSTR args)
{
	Measure* measure = (Measure*)data;
	//RmLog(LOG_DEBUG, L"Command Plugin!!");

	if (!_wcsicmp(args, L"Reload"))
		Read(data, measure->rm);
	else if (!_wcsicmp(args, L"Clear"))
	{
		measure->ret.clear();
		measure->size = 1;
		measure->len = 0;
	}
	else if (!_wcsicmp(args, L"Open"))
	{
		if (!measure->pathname.empty())
		{
			wstring bang = L"[\"\"]";
			bang.insert(2, measure->pathname);
			//RmLog(4, bang.c_str());
			RmExecute(RmGetSkin(measure->rm), bang.c_str());
		}
	}
		
	
}

PLUGIN_EXPORT void Finalize(void* data)
{
	Measure* measure = (Measure*)data;
	delete measure;
	//RmLog(LOG_DEBUG, L"Finalized!!");
}
