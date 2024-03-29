/*
language.cpp

������ � lng �������
*/
/*
Copyright � 1996 Eugene Roshal
Copyright � 2000 Far Group
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "headers.hpp"
#pragma hdrstop

#include "language.hpp"
#include "vmenu2.hpp"
#include "manager.hpp"
#include "message.hpp"
#include "config.hpp"
#include "strmix.hpp"
#include "filestr.hpp"
#include "interf.hpp"
#include "lasterror.hpp"

const wchar_t LangFileMask[] = L"*.lng";

bool OpenLangFile(os::fs::file& LangFile, const string& Path,const string& Mask,const string& Language, string &strFileName, uintptr_t &nCodePage, bool StrongLang,string *pstrLangName)
{
	strFileName.clear();
	string strEngFileName;
	string strLangName;

	auto PathWithSlash = Path;
	AddEndSlash(PathWithSlash);
	FOR(const auto& FindData, os::fs::enum_file(PathWithSlash + Mask))
	{
		strFileName = PathWithSlash + FindData.strFileName;

		if (!LangFile.Open(strFileName, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING))
		{
			strFileName.clear();
		}
		else
		{
			GetFileFormat(LangFile, nCodePage, nullptr, false);

			if (GetLangParam(LangFile,L"Language",&strLangName,nullptr, nCodePage) && !StrCmpI(strLangName, Language))
				break;

			LangFile.Close();

			if (StrongLang)
			{
				strFileName.clear();
				strEngFileName.clear();
				break;
			}

			if (!StrCmpI(strLangName.data(),L"English"))
				strEngFileName = strFileName;
		}
	}

	if (!LangFile.Opened())
	{
		if (!strEngFileName.empty())
			strFileName = strEngFileName;

		if (!strFileName.empty())
		{
			LangFile.Open(strFileName, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING);

			if (pstrLangName)
				*pstrLangName=L"English";
		}
	}

	return LangFile.Opened();
}


int GetLangParam(os::fs::file& LangFile,const string& ParamName,string *strParam1, string *strParam2, UINT nCodePage)
{
	string strFullParamName = L".";
	strFullParamName += ParamName;
	int Length=(int)strFullParamName.size();
	/* $ 29.11.2001 DJ
	   �� ������� ������� � �����; ������ @Contents �� ������
	*/
	BOOL Found = FALSE;
	const auto OldPos = LangFile.GetPointer();

	string ReadStr;
	GetFileString GetStr(LangFile, nCodePage);
	while (GetStr.GetString(ReadStr))
	{
		if (!StrCmpNI(ReadStr.data(), strFullParamName.data(), Length))
		{
			size_t Pos = ReadStr.find(L'=');

			if (Pos != string::npos)
			{
				*strParam1 = ReadStr.substr(Pos + 1);

				if (strParam2)
					strParam2->clear();

				size_t pos = strParam1->find(L',');

				if (pos != string::npos)
				{
					if (strParam2)
					{
						*strParam2 = *strParam1;
						strParam2->erase(0, pos+1);
						RemoveTrailingSpaces(*strParam2);
					}

					strParam1->resize(pos);
				}

				RemoveTrailingSpaces(*strParam1);
				Found = TRUE;
				break;
			}
		}
		else if (!StrCmpNI(ReadStr.data(), L"@Contents", 9))
			break;
	}

	LangFile.SetPointer(OldPos, nullptr, FILE_BEGIN);
	return Found;
}

static bool SelectLanguage(bool HelpLanguage)
{
	const wchar_t *Title,*Mask;
	StringOption *strDest;

	if (HelpLanguage)
	{
		Title=MSG(MHelpLangTitle);
		Mask=Global->HelpFileMask;
		strDest=&Global->Opt->strHelpLanguage;
	}
	else
	{
		Title=MSG(MLangTitle);
		Mask=LangFileMask;
		strDest=&Global->Opt->strLanguage;
	}

	auto LangMenu = VMenu2::create(Title, nullptr, 0, ScrY - 4);
	LangMenu->SetMenuFlags(VMENU_WRAPMODE);
	LangMenu->SetPosition(ScrX/2-8+5*HelpLanguage,ScrY/2-4+2*HelpLanguage,0,0);

	auto PathWithSlash = Global->g_strFarPath;
	AddEndSlash(PathWithSlash);
	FOR(const auto& FindData, os::fs::enum_file(PathWithSlash + Mask))
	{
		os::fs::file LangFile;
		if (!LangFile.Open(PathWithSlash + FindData.strFileName, FILE_READ_DATA, FILE_SHARE_READ, nullptr, OPEN_EXISTING))
			continue;

		uintptr_t nCodePage=CP_OEMCP;
		GetFileFormat(LangFile, nCodePage, nullptr, false);
		string strLangName, strLangDescr;

		if (GetLangParam(LangFile,L"Language",&strLangName,&strLangDescr,nCodePage))
		{
			string strEntryName;

			if (!HelpLanguage || (!GetLangParam(LangFile,L"PluginContents",&strEntryName,nullptr,nCodePage) &&
			                      !GetLangParam(LangFile,L"DocumentContents",&strEntryName,nullptr,nCodePage)))
			{
				MenuItemEx LangMenuItem(str_printf(L"%.40s", !strLangDescr.empty() ? strLangDescr.data():strLangName.data()));

				/* $ 01.08.2001 SVS
				   �� ��������� ����������!
				   ���� � ������� � ����� �������� ��� ���� HLF � �����������
				   ������, ��... ����� ���������� ��� ������ �����.
				*/
				if (LangMenu->FindItem(0,LangMenuItem.strName,LIFIND_EXACTMATCH) == -1)
				{
					LangMenuItem.SetSelect(!StrCmpI(*strDest, strLangName));
					LangMenu->SetUserData(strLangName.data(), (strLangName.size()+1)*sizeof(wchar_t), LangMenu->AddItem(LangMenuItem));
				}
			}
		}
	}

	LangMenu->AssignHighlights(FALSE);
	LangMenu->Run();

	if (LangMenu->GetExitCode()<0)
		return false;

	*strDest = static_cast<const wchar_t*>(LangMenu->GetUserData(nullptr, 0));
	return true;
}

bool SelectInterfaceLanguage() {return SelectLanguage(false);}
bool SelectHelpLanguage() {return SelectLanguage(true);}


/* $ 01.09.2000 SVS
  + ����� �����, ��� ��������� ���������� ��� .Options
   .Options <KeyName>=<Value>
*/
int GetOptionsParam(os::fs::file& SrcFile,const wchar_t *KeyName,string &strValue, UINT nCodePage)
{
	int Length=StrLength(L".Options");
	const auto CurFilePos = SrcFile.GetPointer();
	string ReadStr;
	GetFileString GetStr(SrcFile, nCodePage);
	while (GetStr.GetString(ReadStr))
	{
		if (!StrCmpNI(ReadStr.data(), L".Options", Length))
		{
			string strFullParamName = ReadStr.substr(Length);
			RemoveExternalSpaces(strFullParamName);
			size_t pos = strFullParamName.rfind(L'=');
			if (pos != string::npos)
			{
				strValue = strFullParamName;
				strValue.erase(0, pos+1);
				RemoveExternalSpaces(strValue);
				strFullParamName.resize(pos);
				RemoveExternalSpaces(strFullParamName);

				if (!StrCmpI(strFullParamName.data(),KeyName))
				{
					SrcFile.SetPointer(CurFilePos, nullptr, FILE_BEGIN);
					return TRUE;
				}
			}
		}
	}

	SrcFile.SetPointer(CurFilePos, nullptr, FILE_BEGIN);
	return FALSE;
}

static string ConvertString(const wchar_t *Src, size_t size)
{
	string strDest;
	strDest.reserve(size);

	while (*Src)
	{
		switch (*Src)
		{
		case L'\\':
			switch (Src[1])
			{
			case L'\\':
				strDest.push_back(L'\\');
				Src+=2;
				break;
			case L'\"':
				strDest.push_back(L'\"');
				Src+=2;
				break;
			case L'n':
				strDest.push_back(L'\n');
				Src+=2;
				break;
			case L'r':
				strDest.push_back(L'\r');
				Src+=2;
				break;
			case L'b':
				strDest.push_back(L'\b');
				Src+=2;
				break;
			case L't':
				strDest.push_back('\t');
				Src+=2;
				break;
			default:
				strDest.push_back(L'\\');
				Src++;
				break;
			}
			break;
		case L'"':
			strDest.push_back(L'"');
			Src+=(Src[1]==L'"') ? 2:1;
			break;
		default:
			strDest.push_back(*(Src++));
			break;
		}
	}

	return strDest;
}

void Language::init(const string& Path, int CountNeed)
{
	SCOPED_ACTION(GuardLastError);

	uintptr_t nCodePage = CP_OEMCP;
	string strLangName = Global->Opt->strLanguage.Get();
	os::fs::file LangFile;

	if (!OpenLangFile(LangFile, Path, LangFileMask, Global->Opt->strLanguage, m_FileName, nCodePage, false, &strLangName))
	{
		throw std::runtime_error("Cannot find language data");
	}

	GetFileString GetStr(LangFile, nCodePage);

	if (CountNeed != -1)
	{
		reserve(CountNeed);
	}

	string Buffer;
	while (GetStr.GetString(Buffer))
	{
		RemoveExternalSpaces(Buffer);

		if (Buffer.empty() || Buffer.front() != L'\"')
			continue;

		if (Buffer.size() > 1 && Buffer.back() == L'\"')
		{
			Buffer.pop_back();
		}

		add(ConvertString(Buffer.data() + 1, Buffer.size() - 1));
	}

	//   �������� �������� �� ���������� ����� � LNG-������
	if (CountNeed != -1 && CountNeed != static_cast<int>(size()))
	{
		throw std::runtime_error("Language data is incorrect or damaged");
	}
}

bool Language::CheckMsgId(LNGID MsgId) const
{
	/* $ 19.03.2002 DJ
	   ��� ������������� ������� - ����� ������� ��������� �� ������
	   (��� �����, ��� ���������)
	*/
	if (MsgId >= static_cast<int>(size()) || MsgId < 0)
	{
		/* $ 26.03.2002 DJ
		   ���� �������� ��� � ����� - ��������� �� �������
		*/
		if (!Global->WindowManager->ManagerIsDown())
		{
			/* $ 03.09.2000 IS
			   ! ���������� ��������� �� ���������� ������ � �������� �����
			     (������ ��� ����� ���������� ������ � ����������� ������ ������ - �
			     ����� �� ����� ������)
			*/

			// TODO: localization
			string strMsg1(L"Incorrect or damaged ");
			strMsg1 += m_FileName;
			/* IS $ */
			if (Message(MSG_WARNING, 2,
				L"Error",
				strMsg1.data(),
				(L"Message " + std::to_wstring(MsgId) + L" not found").data(),
				L"Ok", L"Quit")==1)
				exit(0);
		}

		return false;
	}

	return true;
}

const wchar_t* Language::GetMsg(LNGID nID) const
{
	return CheckMsgId(nID)? m_Messages[nID].data() : L"";
}
