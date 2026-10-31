Unicode true
!include "MUI2.nsh"

!ifndef VERSION
!define VERSION "0.0.0"
!endif
!ifndef DIST
!define DIST "dist"
!endif
!ifndef OUTDIR
!define OUTDIR "."
!endif

Name "Ostara"
OutFile "${OUTDIR}\Ostara.exe"
InstallDir "$LOCALAPPDATA\Programs\Ostara"
RequestExecutionLevel user
SetCompressor /SOLID lzma

!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "French"

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${DIST}\*"

  CreateShortCut "$SMPROGRAMS\Ostara.lnk" "$INSTDIR\Ostara.exe"
  CreateShortCut "$DESKTOP\Ostara.lnk" "$INSTDIR\Ostara.exe"

  WriteUninstaller "$INSTDIR\Uninstall.exe"

  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara" "DisplayName" "Ostara"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara" "DisplayVersion" "${VERSION}"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara" "Publisher" "Sophie Denys"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara" "UninstallString" "$INSTDIR\Uninstall.exe"
  WriteRegStr HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara" "DisplayIcon" "$INSTDIR\Ostara.exe"
SectionEnd

Section "Uninstall"
  Delete "$SMPROGRAMS\Ostara.lnk"
  Delete "$DESKTOP\Ostara.lnk"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara"
  RMDir /r "$INSTDIR"
SectionEnd
