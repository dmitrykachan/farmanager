# Microsoft Developer Studio Project File - Name="RightClick" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=RightClick - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "RightClick.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "RightClick.mak" CFG="RightClick - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "RightClick - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RightClick - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "RightClick - Win32 Release for SoftIce" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""$/RightClick/VC6", ECAAAAAA"
# PROP Scc_LocalPath "."
CPP=xicl6.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "RightClick - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /YX /FD /c
# ADD CPP /nologo /Zp1 /W3 /O1 /Ob2 /D "NDEBUG" /D WINVER=0x0400 /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /FAcs /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libctiny.lib /nologo /base:"0x65850000" /subsystem:console /dll /map /machine:I386 /OPT:nowin98 /stub:minstub.exe
# SUBTRACT LINK32 /pdb:none

!ELSEIF  "$(CFG)" == "RightClick - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /Zp1 /MDd /W3 /Gm /GX /Zi /Od /D "_DEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /D WINVER=0x0400 /FAcs /FR /YX /FD /GZ /c
# SUBTRACT CPP /WX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "_DEBUG"
# ADD RSC /l 0x419 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /base:"0x65850000" /subsystem:console /dll /map /debug /machine:I386 /pdbtype:sept
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
OutDir=.\Debug
TargetName=RightClick
SOURCE="$(InputPath)"
PostBuild_Desc=Copying plugin to the FAR folder
PostBuild_Cmds=copy "$(OutDir)\$(TargetName).dll" "c:\Program Files\Far\Plugins\RightClick"	copy C:\Projects\RightClick\LngHlf\*.lng "c:\Program Files\Far\Plugins\RightClick"	copy C:\Projects\RightClick\LngHlf\*.hlf "c:\Program Files\Far\Plugins\RightClick"
# End Special Build Tool

!ELSEIF  "$(CFG)" == "RightClick - Win32 Release for SoftIce"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "RightClick___Win32_Release_for_SoftIce"
# PROP BASE Intermediate_Dir "RightClick___Win32_Release_for_SoftIce"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release_for_SoftIce"
# PROP Intermediate_Dir "Release_for_SoftIce"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /Zp1 /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /YX /FD /c
# SUBTRACT BASE CPP /Fr
# ADD CPP /nologo /Zp1 /W3 /Zi /O1 /D "NDEBUG" /D "WIN32" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "RightClick_EXPORTS" /D WINVER=0x0400 /Fr /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x419 /d "NDEBUG"
# ADD RSC /l 0x419 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=xilink6.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386 /OPT:nowin98
# SUBTRACT BASE LINK32 /pdb:none
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib libctiny.lib /nologo /base:"0x65850000" /subsystem:console /dll /debug /machine:I386 /OPT:nowin98
# SUBTRACT LINK32 /pdb:none
# Begin Special Build Tool
OutDir=.\Release_for_SoftIce
SOURCE="$(InputPath)"
PostBuild_Desc=Making NMS...
PostBuild_Cmds="E:\Program Files\NuMega\SoftIceNT\loader32.exe" /PACKAGE /translate $(OutDir)\RightClick.dll
# End Special Build Tool

!ENDIF 

# Begin Target

# Name "RightClick - Win32 Release"
# Name "RightClick - Win32 Debug"
# Name "RightClick - Win32 Release for SoftIce"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\auto_sz.cpp
# End Source File
# Begin Source File

SOURCE=..\FarMenu.cpp
# End Source File
# Begin Source File

SOURCE=..\MenuDlg.cpp
# End Source File
# Begin Source File

SOURCE=..\OleThread.cpp
# End Source File
# Begin Source File

SOURCE=..\Pidl.cpp
# End Source File
# Begin Source File

SOURCE=..\Plugin.cpp
# End Source File
# Begin Source File

SOURCE=..\Reg.cpp

!IF  "$(CFG)" == "RightClick - Win32 Release"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "RightClick - Win32 Debug"

# PROP Exclude_From_Build 1

!ELSEIF  "$(CFG)" == "RightClick - Win32 Release for SoftIce"

# PROP BASE Exclude_From_Build 1
# PROP Exclude_From_Build 1

!ENDIF 

# End Source File
# Begin Source File

SOURCE=..\RightClick.cpp
# End Source File
# Begin Source File

SOURCE=..\RightClick.def
# End Source File
# Begin Source File

SOURCE=..\RightClick.rc
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\auto_sz.h
# End Source File
# Begin Source File

SOURCE=..\FarMenu.h
# End Source File
# Begin Source File

SOURCE=..\Handle.h
# End Source File
# Begin Source File

SOURCE=..\HMenu.h
# End Source File
# Begin Source File

SOURCE=..\MenuDlg.h
# End Source File
# Begin Source File

SOURCE=..\OleThread.h
# End Source File
# Begin Source File

SOURCE=..\Pidl.h
# End Source File
# Begin Source File

SOURCE=..\Plugin.h
# End Source File
# Begin Source File

SOURCE=..\plugin.hpp
# End Source File
# Begin Source File

SOURCE=..\Resource.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# Begin Source File

SOURCE=..\Stuff\ToDo.txt
# End Source File
# End Target
# End Project
