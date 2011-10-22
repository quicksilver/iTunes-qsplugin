# Microsoft Developer Studio Project File - Name="libopendaap" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Dynamic-Link Library" 0x0102

CFG=libopendaap - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libopendaap.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libopendaap.mak" CFG="libopendaap - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libopendaap - Win32 Release" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE "libopendaap - Win32 Debug" (based on "Win32 (x86) Dynamic-Link Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libopendaap - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBOPENDAAP_EXPORTS" /YX /FD /c
# ADD CPP /nologo /MT /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBOPENDAAP_EXPORTS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "NDEBUG"
# ADD RSC /l 0xc09 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /machine:I386

!ELSEIF  "$(CFG)" == "libopendaap - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "libopendaap___Win32_Debug"
# PROP BASE Intermediate_Dir "libopendaap___Win32_Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "libopendaap___Win32_Debug"
# PROP Intermediate_Dir "libopendaap___Win32_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBOPENDAAP_EXPORTS" /YX /FD /GZ /c
# ADD CPP /nologo /MTd /W3 /Gm /GX /ZI /Od /I ".\mDNS" /I ".\mDNS\win32" /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_USRDLL" /D "LIBOPENDAAP_EXPORTS" /FD /GZ /c
# SUBTRACT CPP /YX
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0xc09 /d "_DEBUG"
# ADD RSC /l 0xc09 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept
# ADD LINK32 kernel32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib ws2_32.lib /nologo /dll /debug /machine:I386 /pdbtype:sept

!ENDIF 

# Begin Target

# Name "libopendaap - Win32 Release"
# Name "libopendaap - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Group "mDNS Source"

# PROP Default_Filter ".c"
# Begin Source File

SOURCE=.\mDNS\win32\DNSServiceDiscovery.c
# End Source File
# Begin Source File

SOURCE=.\mDNS\win32\DNSServices.c
# End Source File
# Begin Source File

SOURCE=.\mDNS\mDNS.c

!IF  "$(CFG)" == "libopendaap - Win32 Release"

!ELSEIF  "$(CFG)" == "libopendaap - Win32 Debug"

# ADD CPP /I ".\\"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mDNS\win32\mDNSWin32.c
# End Source File
# End Group
# Begin Source File

SOURCE=.\client.c
# End Source File
# Begin Source File

SOURCE=.\daap.c
# End Source File
# Begin Source File

SOURCE=.\debug\debug.c
# End Source File
# Begin Source File

SOURCE=.\discover_dnsservice.c
# End Source File
# Begin Source File

SOURCE=.\dmap_generics.c
# End Source File
# Begin Source File

SOURCE=.\global.c
# End Source File
# Begin Source File

SOURCE=.\authentication\hasher.c

!IF  "$(CFG)" == "libopendaap - Win32 Release"

!ELSEIF  "$(CFG)" == "libopendaap - Win32 Debug"

# ADD CPP /I "."

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\http_client.c
# End Source File
# Begin Source File

SOURCE=.\authentication\md5.c

!IF  "$(CFG)" == "libopendaap - Win32 Release"

!ELSEIF  "$(CFG)" == "libopendaap - Win32 Debug"

# ADD CPP /I "."

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Group "mDNS Header"

# PROP Default_Filter ".h"
# Begin Source File

SOURCE=.\mDNS\win32\DNSServiceDiscovery.h
# End Source File
# Begin Source File

SOURCE=.\mDNS\win32\DNSServices.h
# End Source File
# Begin Source File

SOURCE=.\mDNS\mDNSClientAPI.h
# End Source File
# Begin Source File

SOURCE=.\mDNS\mDNSDebug.h
# End Source File
# Begin Source File

SOURCE=.\mDNS\mDNSPlatformFunctions.h
# End Source File
# Begin Source File

SOURCE=.\mDNS\win32\mDNSWin32.h
# End Source File
# End Group
# Begin Source File

SOURCE=.\client.h
# End Source File
# Begin Source File

SOURCE=.\config.h
# End Source File
# Begin Source File

SOURCE=.\daap.h
# End Source File
# Begin Source File

SOURCE=.\daap_contentcodes.h
# End Source File
# Begin Source File

SOURCE=.\daap_readtypes.h
# End Source File
# Begin Source File

SOURCE=.\debug\debug.h
# End Source File
# Begin Source File

SOURCE=.\discover.h
# End Source File
# Begin Source File

SOURCE=.\dmap_generics.h
# End Source File
# Begin Source File

SOURCE=.\authentication\hasher.h
# End Source File
# Begin Source File

SOURCE=.\http_client.h
# End Source File
# Begin Source File

SOURCE=.\authentication\md5.h
# End Source File
# Begin Source File

SOURCE=.\portability.h
# End Source File
# Begin Source File

SOURCE=.\private.h
# End Source File
# Begin Source File

SOURCE=.\thread.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# End Group
# End Target
# End Project
