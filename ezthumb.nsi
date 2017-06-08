; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

; The name of the installer
Name "Ezthumb Installer"

; The file to write
;OutFile "ezthumb-3.2.3-setup.exe"
!include "nsis_version.txt"

; The default installation directory
InstallDir $PROGRAMFILES\Ezthumb

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\ezthumb" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

; The stuff to install
Section "Ezthumb (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "ezthumb.exe"
  File "EzthumbWin.exe"
  File "ezthumb.1"
  File "ezthumb.pdf"
  File "ezthumb.ico"
  File "ezqiz"
  File "COPYING"
  File "ChangeLog"
  File "Readme.txt"
  File "TODO"
!ifdef win7
  File "libmingw\ffmpeg\bin\*.dll"
!else
  File "libmingw\ffmpeg_xp\bin\*.dll"
!endif
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\ezthumb "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ezthumb" "DisplayName" "Ezthumb Uninstaller"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ezthumb" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ezthumb" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ezthumb" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Ezthumb"
  CreateShortCut "$SMPROGRAMS\Ezthumb\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Ezthumb\ezthumb.lnk" "$INSTDIR\EzthumbWin.exe" "" "$INSTDIR\EzthumbWin.exe" 0
  
SectionEnd

; http://nsis.sourceforge.net/Windows_7_Cascading_Context_Menu
; https://github.com/NSIS-Dev/Windows-7-Context-Menu/blob/master/installer.nsi
; http://stackoverflow.com/questions/370114/how-can-i-add-a-context-menu-to-the-windows-explorer-for-a-java-application
; https://www.howtogeek.com/107965/how-to-add-any-application-shortcut-to-windows-explorers-context-menu/
; https://msdn.microsoft.com/en-us/library/bb166549.aspx
Section "Add to Context Menu"
  WriteRegStr HKCR "Applications\Ezthumb.exe" "" "Ezthumb"
  WriteRegStr HKCR "Applications\Ezthumb.exe\shell\open\command" "" "$INSTDIR\EzthumbWin.exe --gui-progress $\"%1$\""

  WriteRegStr HKCR "SystemFileAssociations\video\OpenWithList\Ezthumb.exe" "" ""
  WriteRegStr HKCR "SystemFileAssociations\video\shell\Run Ezthumb\command" "" "$INSTDIR\EzthumbWin.exe --gui-progress $\"%1$\""

  ; Prepared for the folder operation --  not now
  ; WriteRegStr HKCR "Directory\shell\Run Ezthumb\command" "" "$INSTDIR\EzthumbWin.exe --gui-progress $\"%1$\""
  ; WriteRegStr HKCR "Directory\Background\shell\Run Ezthumb\command" "" "$INSTDIR\EzthumbWin.exe --gui-progress $\"%1$\""
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Ezthumb"
  DeleteRegKey HKLM SOFTWARE\Ezthumb
  DeleteRegKey HKCU SOFTWARE\ezthumb

  ; delete the context menu
  DeleteRegKey HKCR "SystemFileAssociations\video\OpenWithList\Ezthumb.exe"
  DeleteRegKey HKCR "SystemFileAssociations\video\shell\Run Ezthumb"
  DeleteRegKey HKCR "Directory\shell\Run Ezthumb"
  DeleteRegKey HKCR "Directory\Background\shell\Run Ezthumb"
  DeleteRegKey HKCR "Applications\Ezthumb.exe"

  ; Remove files and uninstaller
  Delete $INSTDIR\*.*

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Ezthumb\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Ezthumb"
  RMDir "$INSTDIR"

SectionEnd
