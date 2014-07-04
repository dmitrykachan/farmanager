#include <CRT/crt.hpp>
#include <objbase.h>
#include <shlobj.h>
#include <plugin.hpp>

#include "FARCmds.hpp"
#include "Lang.hpp"
#include <initguid.h>
#include "guid.hpp"


const wchar_t *GetMsg(int MsgId)
{
	return Info.GetMsg(&MainGuid,MsgId);
}

// need "delete[]"
wchar_t *ExpandEnv(const wchar_t* Src, DWORD* Length)
{
	DWORD sizeExp=ExpandEnvironmentStrings(Src,NULL,0);
	wchar_t *temp=new wchar_t[sizeExp+1];
	if (temp)
		ExpandEnvironmentStrings(Src,temp,sizeExp);
	else
		sizeExp=0;

	if (Length)
		*Length=sizeExp;

	return temp;
}

/*
 �����頥� �᫮, ��१�� ��� �� ��ப�, ��� -2 � ��砥 �訡��
 Start, End - ��砫� � ����� ��ப�
*/
int GetInt(wchar_t *Start, wchar_t *End)
{
	int Ret=-2;

	if (End >= Start)
	{
		wchar_t Tmp[16];
		int Size=(int)(End-Start);

		if (Size)
		{
			if (Size < 11)
			{
				wmemcpy(Tmp,Start,Size);
				Tmp[Size]=0;
				Ret=FSF.atoi(Tmp);
			}
		}
		else
			Ret=0;
	}

	return Ret;
}

// �������� � ��ப� Str Count �宦����� �����ப� FindStr �� �����ப� ReplStr
// �᫨ Count < 0 - �������� "�� ������ ������"
// Return - ������⢮ �����
int ReplaceStrings(wchar_t *Str,const wchar_t *FindStr,const wchar_t *ReplStr,int Count,BOOL IgnoreCase)
{
	int I=0, J=0, Res;
	int LenReplStr=(int)lstrlen(ReplStr);
	int LenFindStr=(int)lstrlen(FindStr);
	int L=(int)lstrlen(Str);

	while (I <= L-LenFindStr)
	{
		Res=IgnoreCase?_memicmp(Str+I, FindStr, LenFindStr*sizeof(wchar_t)):memcmp(Str+I, FindStr, LenFindStr*sizeof(wchar_t));

		if (Res == 0)
		{
			if (LenReplStr > LenFindStr)
				wmemmove(Str+I+(LenReplStr-LenFindStr),Str+I,lstrlen(Str+I)+1); // >>
			else if (LenReplStr < LenFindStr)
				wmemmove(Str+I,Str+I+(LenFindStr-LenReplStr),lstrlen(Str+I+(LenFindStr-LenReplStr))+1); //??

			wmemcpy(Str+I,ReplStr,LenReplStr);
			I += LenReplStr;

			if (++J == Count && Count > 0)
				break;
		}
		else
			I++;

		L=(int)lstrlen(Str);
	}

	return J;
}


/*
 �����頥� PipeFound
*/
int PartCmdLine(const wchar_t *CmdStr,wchar_t *NewCmdStr,int SizeNewCmdStr,wchar_t *NewCmdPar,int SizeNewCmdPar)
{
	int PipeFound = FALSE;

	if (NewCmdStr)
		*NewCmdStr=0;
	if (NewCmdPar)
		*NewCmdPar=0;

	wchar_t *Temp=ExpandEnv(CmdStr,nullptr);

	if (Temp)
	{
		FSF.Trim(Temp);
		wchar_t *CmdPtr = Temp;
		wchar_t *ParPtr = NULL;
		int QuoteFound = FALSE;

		// �������� ᮡ�⢥��� ������� ��� �ᯮ������ � ��ࠬ����.
		// �� �⮬ ������ ��।���� ����稥 ᨬ����� ��८�।������ ��⮪��
		// ����⠥� � ��⮬ ����祪. �.�. ���� � ����窠� - �� ����.

		while (*CmdPtr)
		{
			if (*CmdPtr == L'"')
				QuoteFound = !QuoteFound;

			if (!QuoteFound && CmdPtr != Temp)
			{
				if (*CmdPtr == L'>' || *CmdPtr == L'<' ||
				        *CmdPtr == L'|' || *CmdPtr == L' ' ||
				        *CmdPtr == L'/' ||      // ��ਠ�� "far.exe/?"
				        *CmdPtr == L'&'
				   )
				{
					if (!ParPtr)
						ParPtr = CmdPtr;

					if (*CmdPtr != L' ' && *CmdPtr != L'/')
						PipeFound = TRUE;
				}
			}

			if (ParPtr && PipeFound) // ��� ����� ��祣� �� ���� 㧭�����
				break;

			CmdPtr++;
		}

		if (NewCmdPar && ParPtr) // �� ��諨 ��ࠬ���� � �⤥�塞 ��� �� ��⫥�
		{
			if (*ParPtr == L' ') //AY: ���� �஡�� ����� �������� � ��ࠬ��ࠬ� �� �㦥�,
				*(ParPtr++)=0;     //    �� ���������� ������ � Execute.

			lstrcpyn(NewCmdPar, ParPtr, SizeNewCmdPar-1);
			*ParPtr = 0;
		}

		if (NewCmdStr)
		{
			lstrcpyn(NewCmdStr, Temp, SizeNewCmdStr-1);
			FSF.Unquote(NewCmdStr);
		}

		delete[] Temp;
	}

	return PipeFound;
}

BOOL ProcessOSAliases(wchar_t *Str,int SizeStr)
{
	typedef DWORD (WINAPI *PGETCONSOLEALIAS)(
	    wchar_t *lpSource,          // in
	    wchar_t *lpTargetBuffer,    // out
	    DWORD TargetBufferLength,   // in
	    wchar_t *lpExeName          // in
	);
	static PGETCONSOLEALIAS pGetConsoleAlias=NULL;

	if (!pGetConsoleAlias)
	{
		pGetConsoleAlias = (PGETCONSOLEALIAS)GetProcAddress(GetModuleHandleW(L"kernel32"),"GetConsoleAliasW");

		if (!pGetConsoleAlias)
			return FALSE;
	}

	wchar_t NewCmdStr[4096];
	wchar_t NewCmdPar[4096];

	PartCmdLine(Str,NewCmdStr,ARRAYSIZE(NewCmdStr),NewCmdPar,ARRAYSIZE(NewCmdPar));

	DWORD SizeModuleName = 0;
	wchar_t *ModuleName=nullptr;
	// <GetModuleFileName>
	{
		DWORD BufferSize = MAX_PATH;

		do {
			BufferSize *= 2;
			wchar_t *ModuleNameTemp=(wchar_t*)realloc(ModuleName,BufferSize);
			if (ModuleNameTemp)
			{
				ModuleName=ModuleNameTemp;
				SizeModuleName = GetModuleFileName(NULL, ModuleName, BufferSize);
			}
		} while ((SizeModuleName >= BufferSize) || (!SizeModuleName && GetLastError() == ERROR_INSUFFICIENT_BUFFER));
	}

	int ret=pGetConsoleAlias(NewCmdStr,NewCmdStr,sizeof(NewCmdStr),(wchar_t*)FSF.PointToName(ModuleName));

	if (!ret)
	{
		wchar_t *CSModuleName=ExpandEnv(L"%COMSPEC%",nullptr);
		if (CSModuleName)
		{
			ret=pGetConsoleAlias(NewCmdStr,NewCmdStr,sizeof(NewCmdStr),(wchar_t*)FSF.PointToName(CSModuleName));
			delete[] CSModuleName;
		}
	}

	if (!ret)
	{
		if (ModuleName)
			free(ModuleName);
		return FALSE;
	}

	if (!ReplaceStrings(NewCmdStr,L"$*",NewCmdPar,-1,FALSE))
	{
		lstrcat(NewCmdStr,L" ");
		lstrcat(NewCmdStr,NewCmdPar);
	}

	lstrcpyn(Str,NewCmdStr,SizeStr-1);

	if (ModuleName)
		free(ModuleName);

	return TRUE;
}


wchar_t *GetShellLinkPath(const wchar_t *LinkFile)
{
	bool Result=false;
	wchar_t *Path=nullptr;

	wchar_t *Temp=ExpandEnv(LinkFile,nullptr);
	if (!Temp)
		return nullptr;

	FSF.Unquote(Temp);
	size_t SizeNativePath=FSF.ConvertPath(CPM_NATIVE, Temp, nullptr, 0);
	wchar_t *FileName=new wchar_t[SizeNativePath+1];
	if (!FileName)
	{
		delete[] Temp;
		return nullptr;
	}
	FSF.ConvertPath(CPM_NATIVE, Temp, FileName, SizeNativePath);
	delete[] Temp;

	if (!(*FileName && FileExists(FileName)))
	{
		delete[] FileName;
		return nullptr;
	}

	// <Check lnk-header>
	HANDLE hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL );

	if (hFile != INVALID_HANDLE_VALUE)
	{
		struct ShellLinkHeader
		{
			DWORD    HeaderSize;
			BYTE     LinkCLSID[16];
			DWORD    LinkFlags;
			DWORD    FileAttributes;
			FILETIME CreationTime;
			FILETIME AccessTime;
			FILETIME WriteTime;
			DWORD    FileSize;
			DWORD    IconIndex;
			DWORD    ShowCommand;
			WORD     HotKey;
			WORD     Reserved1;
			DWORD    Reserved2;
			DWORD    Reserved3;
		};

		ShellLinkHeader slh = { 0 };
		DWORD read = 0;
		ReadFile( hFile, &slh, sizeof( ShellLinkHeader ), &read, NULL );

		if ( read == sizeof( ShellLinkHeader ) && slh.HeaderSize == 0x0000004C)
		{
			if (!memcmp( slh.LinkCLSID, "\x01\x14\x02\x00\x00\x00\x00\x00\xC0\x00\x00\x00\x00\x00\x00\x46\x9b", 16 ))
				Result=true;
		}

		CloseHandle( hFile );
	}
	// </Check lnk-header>

	if (Result)
	{
		// <get target>
		Result=false;
		/*HRESULT hres0 = */CoInitialize(NULL);

		IShellLink* psl = NULL;
		HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
		if (SUCCEEDED(hres))
		{
			IPersistFile* ppf = NULL;
			hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
			if (SUCCEEDED(hres))
			{
				hres = ppf->Load(FileName, STGM_READ);
				if (SUCCEEDED(hres))
				{
					hres = psl->Resolve(NULL, 0);
					if (SUCCEEDED(hres))
					{
						wchar_t TargPath[MAX_PATH] = {0};
						hres = psl->GetPath(TargPath, ARRAYSIZE(TargPath), NULL, SLGP_RAWPATH);
						if (SUCCEEDED(hres))
						{
							Path=new wchar_t[lstrlen(TargPath)+1];
							if (Path)
								lstrcpy(Path, TargPath);
						}
					}
				}
				ppf->Release();
			}
			psl->Release();
		}

		CoUninitialize();
		// </get target>
	}

	delete[] FileName;

	return Path;
}

bool StrToGuid(const wchar_t *Value,GUID *Guid)
{
	return UuidFromString(reinterpret_cast<unsigned short*>((void*)Value), Guid) == RPC_S_OK;
}
