Unicode true

; L'icône n'est pas dans DIST : elle est compilée dans Ostara.exe, et NSIS veut un
; fichier .ico pour la sienne. Le chemin par défaut est relatif à la racine du dépôt,
; d'où makensis est lancé en CI ; la CI le passe explicitement.
!ifndef APPICON
!define APPICON "share\icons\app\ostara.ico"
!endif

; MUI2 pose SA propre icône par défaut, et il le fait au premier MUI_PAGE : les
; !define doivent donc être là avant l'include, sinon ils ne servent à rien.
!define MUI_ICON "${APPICON}"
!define MUI_UNICON "${APPICON}"

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

; Icône des deux exécutables — l'installeur que la cliente télécharge et le
; désinstalleur. Après les MUI_PAGE, pour que MUI2 ne reprenne pas la main : sans
; ces deux lignes, les deux portent l'icône NSIS générique.
Icon "${APPICON}"
UninstallIcon "${APPICON}"

; ═══════════════════════════════════════════════════════════════════════════
; Types de fichiers
;
; Windows ne connaît l'icône d'un type que par le registre : l'icône compilée dans
; l'exécutable ne sert qu'aux raccourcis, à la barre des tâches et à la fiche
; « Applications et fonctionnalités ». Sans association, un `.ostara` garde
; l'icône blanche générique et un double-clic n'ouvre rien.
;
; L'installeur est « per user » (RequestExecutionLevel user, $LOCALAPPDATA), donc
; tout va sous HKCU\Software\Classes — jamais HKLM, qui demanderait l'élévation.
;
; Seuls les formats qu'Ostara ouvre comme document sont enregistrés. Les formats
; de Valentina (.val, .vst, .vit) ne le sont pas : on ne les lui prend pas.
; ═══════════════════════════════════════════════════════════════════════════
!macro OstaraTypeRegister EXT PROGID LABEL
  WriteRegStr HKCU "Software\Classes\${EXT}" "" "${PROGID}"
  WriteRegStr HKCU "Software\Classes\${PROGID}" "" "${LABEL}"
  WriteRegStr HKCU "Software\Classes\${PROGID}\DefaultIcon" "" '"$INSTDIR\Ostara.exe",0'
  WriteRegStr HKCU "Software\Classes\${PROGID}\shell\open\command" "" '"$INSTDIR\Ostara.exe" "%1"'
!macroend

!macro OstaraTypeUnregister EXT PROGID
  ; La clé du type d'abord : sinon Windows garde un type qui renvoie à un ProgID disparu.
  DeleteRegKey HKCU "Software\Classes\${EXT}"
  DeleteRegKey HKCU "Software\Classes\${PROGID}"
!macroend

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

  !insertmacro OstaraTypeRegister ".ostara" "Ostara.Pattern" "Ostara pattern"
  !insertmacro OstaraTypeRegister ".omt" "Ostara.Measurements" "Ostara measurements table"
  !insertmacro OstaraTypeRegister ".otu" "Ostara.Measurements" "Ostara measurements table"
  !insertmacro OstaraTypeRegister ".oct" "Ostara.Cartouche" "Ostara cartouche template"

  ; L'Explorateur garde les icônes et les associations en cache : sans cette
  ; notification, le nouveau type n'apparaît qu'après un redémarrage de session.
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'
SectionEnd

Section "Uninstall"
  !insertmacro OstaraTypeUnregister ".ostara" "Ostara.Pattern"
  !insertmacro OstaraTypeUnregister ".omt" "Ostara.Measurements"
  !insertmacro OstaraTypeUnregister ".otu" "Ostara.Measurements"
  !insertmacro OstaraTypeUnregister ".oct" "Ostara.Cartouche"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, i 0, i 0)'

  Delete "$SMPROGRAMS\Ostara.lnk"
  Delete "$DESKTOP\Ostara.lnk"
  DeleteRegKey HKCU "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ostara"
  RMDir /r "$INSTDIR"
SectionEnd
