# Microsoft Developer Studio Project File - Name="WinSpy" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=WinSpy - Win32 Unicode Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "WinSpy.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "WinSpy.mak" CFG="WinSpy - Win32 Unicode Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "WinSpy - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "WinSpy - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinSpy - Win32 Unicode Debug" (based on "Win32 (x86) Application")
!MESSAGE "WinSpy - Win32 Unicode Release" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "WinSpy - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /profile /map /nodefaultlib

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Debug"

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
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /FR /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /profile /debug /machine:I386

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinSpy___Win32_Unicode_Debug"
# PROP BASE Intermediate_Dir "WinSpy___Win32_Unicode_Debug"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Unicode_Debug"
# PROP Intermediate_Dir "Unicode_Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /YX /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "_DEBUG"
# ADD RSC /l 0x809 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinSpy___Win32_Unicode_Release"
# PROP BASE Intermediate_Dir "WinSpy___Win32_Unicode_Release"
# PROP BASE Ignore_Export_Lib 0
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Unicode_Release"
# PROP Intermediate_Dir "Unicode_Release"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /O1 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /D "_UNICODE" /D "UNICODE" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x809 /d "NDEBUG"
# ADD RSC /l 0x809 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 libctiny.lib comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT BASE LINK32 /profile
# ADD LINK32 comctl32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# SUBTRACT LINK32 /profile

!ENDIF 

# Begin Target

# Name "WinSpy - Win32 Release"
# Name "WinSpy - Win32 Debug"
# Name "WinSpy - Win32 Unicode Debug"
# Name "WinSpy - Win32 Unicode Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\BitmapButton.c
# End Source File
# Begin Source File

SOURCE=.\CatpureWindow.c
# End Source File
# Begin Source File

SOURCE=.\DisplayClassInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayGeneralInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayProcessInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayPropInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayScrollInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayStyleInfo.c
# End Source File
# Begin Source File

SOURCE=.\DisplayWindowInfo.c
# End Source File
# Begin Source File

SOURCE=.\EditSize.c
# End Source File
# Begin Source File

SOURCE=.\FindTool.c

!IF  "$(CFG)" == "WinSpy - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\FunkyList.c
# End Source File
# Begin Source File

SOURCE=.\GetRemoteWindowInfo.c
# End Source File
# Begin Source File

SOURCE=.\InjectThread.c

!IF  "$(CFG)" == "WinSpy - Win32 Release"

# ADD CPP /W3

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Debug"

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Debug"

!ELSEIF  "$(CFG)" == "WinSpy - Win32 Unicode Release"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\Options.c
# End Source File
# Begin Source File

SOURCE=.\RegHelper.c
# End Source File
# Begin Source File

SOURCE=.\StaticCtrl.c
# End Source File
# Begin Source File

SOURCE=.\StyleEdit.c
# End Source File
# Begin Source File

SOURCE=.\TabCtrlUtils.c
# End Source File
# Begin Source File

SOURCE=.\Utils.c
# End Source File
# Begin Source File

SOURCE=.\WindowFromPointEx.c
# End Source File
# Begin Source File

SOURCE=.\WinSpy.c
# End Source File
# Begin Source File

SOURCE=.\WinSpy.rc
# End Source File
# Begin Source File

SOURCE=.\WinSpyCommand.c
# End Source File
# Begin Source File

SOURCE=.\WinSpyDlgs.c
# End Source File
# Begin Source File

SOURCE=.\WinSpyTree.c
# End Source File
# Begin Source File

SOURCE=.\WinSpyWindow.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\BitmapButton.h
# End Source File
# Begin Source File

SOURCE=.\CaptureWindow.h
# End Source File
# Begin Source File

SOURCE=.\FindTool.h
# End Source File
# Begin Source File

SOURCE=.\InjectThread.h
# End Source File
# Begin Source File

SOURCE=.\resource.h
# End Source File
# Begin Source File

SOURCE=.\Utils.h
# End Source File
# Begin Source File

SOURCE=.\WindowFromPointEx.h
# End Source File
# Begin Source File

SOURCE=.\WinSpy.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\app.ico
# End Source File
# Begin Source File

SOURCE=.\bitmap1.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap2.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap3.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap4.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap5.bmp
# End Source File
# Begin Source File

SOURCE=.\bitmap6.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00001.bmp
# End Source File
# Begin Source File

SOURCE=.\bmp00002.bmp
# End Source File
# Begin Source File

SOURCE=.\check1.bmp
# End Source File
# Begin Source File

SOURCE=.\check2.bmp
# End Source File
# Begin Source File

SOURCE=.\cursor1.cur
# End Source File
# Begin Source File

SOURCE=.\default1.bin
# End Source File
# Begin Source File

SOURCE=.\dnarrow.ico
# End Source File
# Begin Source File

SOURCE=.\drag1.bmp
# End Source File
# Begin Source File

SOURCE=.\drag2.bmp
# End Source File
# Begin Source File

SOURCE=.\ellipses.ico
# End Source File
# Begin Source File

SOURCE=.\enter.ico
# End Source File
# Begin Source File

SOURCE=.\icon1.ico
# End Source File
# Begin Source File

SOURCE=.\icon10.ico
# End Source File
# Begin Source File

SOURCE=.\icon11.ico
# End Source File
# Begin Source File

SOURCE=.\icon12.ico
# End Source File
# Begin Source File

SOURCE=.\icon13.ico
# End Source File
# Begin Source File

SOURCE=.\icon14.ico
# End Source File
# Begin Source File

SOURCE=.\icon15.ico
# End Source File
# Begin Source File

SOURCE=.\icon16.ico
# End Source File
# Begin Source File

SOURCE=.\icon17.ico
# End Source File
# Begin Source File

SOURCE=.\icon18.ico
# End Source File
# Begin Source File

SOURCE=.\icon2.ico
# End Source File
# Begin Source File

SOURCE=.\icon3.ico
# End Source File
# Begin Source File

SOURCE=.\icon4.ico
# End Source File
# Begin Source File

SOURCE=.\icon5.ico
# End Source File
# Begin Source File

SOURCE=.\icon6.ico
# End Source File
# Begin Source File

SOURCE=.\icon7.ico
# End Source File
# Begin Source File

SOURCE=.\icon8.ico
# End Source File
# Begin Source File

SOURCE=.\icon9.ico
# End Source File
# Begin Source File

SOURCE=.\pin.bmp
# End Source File
# End Group
# End Target
# End Project
