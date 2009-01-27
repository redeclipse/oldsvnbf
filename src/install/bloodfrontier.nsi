Name "BloodFrontier"

OutFile "bloodfrontier-B1-RC1-setup.exe"

InstallDir $PROGRAMFILES\BloodFrontier

InstallDirRegKey HKLM "Software\BloodFrontier" "Install_Dir"

SetCompressor /SOLID lzma
XPStyle on

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Section "BloodFrontier (required)"

  SectionIn RO
  
  SetOutPath $INSTDIR
  
  File /r "..\..\*.*"
  
  WriteRegStr HKLM SOFTWARE\BloodFrontier "Install_Dir" "$INSTDIR"
  
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BloodFrontier" "DisplayName" "BloodFrontier"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BloodFrontier" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BloodFrontier" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BloodFrontier" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\BloodFrontier"
  
  SetOutPath "$INSTDIR"
  
  CreateShortCut "$INSTDIR\BloodFrontier.lnk"                "$INSTDIR\bloodfrontier.bat" "" "$INSTDIR\bloodfrontier.bat" 0
  CreateShortCut "$SMPROGRAMS\BloodFrontier\BloodFrontier.lnk" "$INSTDIR\bloodfrontier.bat" "" "$INSTDIR\bloodfrontier.bat" 0
  CreateShortCut "$SMPROGRAMS\BloodFrontier\Uninstall.lnk"   "$INSTDIR\uninstall.exe"   "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\BloodFrontier\README.lnk"      "$INSTDIR\readme.txt"     "" "$INSTDIR\readme.txt" 0
  
SectionEnd

Section "Uninstall"
  
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\BloodFrontier"
  DeleteRegKey HKLM SOFTWARE\BloodFrontier

  RMDir /r "$SMPROGRAMS\BloodFrontier"
  RMDir /r "$INSTDIR"

SectionEnd
