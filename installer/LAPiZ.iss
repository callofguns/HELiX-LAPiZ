; Windows installer for LAPiZ, built with Inno Setup 6 (https://jrsoftware.org/isinfo.php).
; Build the plugin in Release first, then run from the project folder:
;   "%LOCALAPPDATA%\Programs\Inno Setup 6\ISCC.exe" installer\LAPiZ.iss
; The finished installer is written to dist\.
; Releases are normally built by .github/workflows/release.yml, which passes /DAppVersion=<tag>.
;
; Keep AppVersion in step with Source/Version.h and the VERSION in CMakeLists.txt.
; Never change AppId: Windows uses it to recognise an older LAPiZ when updating or uninstalling,
; and LAPiZ's updater (Source/Updater.cpp) uses it to find where the standalone app is installed.

#ifndef AppVersion
  #define AppVersion "1.4.0"
#endif
#define Artefacts "..\build\Lapiz_artefacts\Release"

[Setup]
AppId={{8AC7DA59-70F4-4FBF-BF06-E7911CD5DCFE}
AppName=LAPiZ
AppVersion={#AppVersion}
AppVerName=LAPiZ {#AppVersion}
AppPublisher=HELiX
AppPublisherURL=https://github.com/callofguns/HELiX-LAPiZ
AppSupportURL=https://github.com/callofguns/HELiX-LAPiZ/issues
AppUpdatesURL=https://github.com/callofguns/HELiX-LAPiZ/releases
DefaultDirName={autopf}\HELiX\LAPiZ
DefaultGroupName=HELiX
DisableProgramGroupPage=yes
LicenseFile=..\LICENSE
OutputDir=..\dist
OutputBaseFilename=LAPiZ-{#AppVersion}-setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
; The VST3 folder is shared by every plugin on the computer, so installing needs administrator rights.
; (Updates after that don't: see [Dirs].)
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName=LAPiZ (HELiX)
UninstallDisplayIcon={app}\LAPiZ.exe
; Close FL Studio / LAPiZ if they have the plugin open, so the files can be replaced.
CloseApplications=yes

[Types]
Name: "full"; Description: "Plugin and standalone app"
Name: "plugin"; Description: "Plugin only (VST3)"
Name: "custom"; Description: "Choose what to install"; Flags: iscustom

[Components]
Name: "vst3"; Description: "LAPiZ VST3 plugin (for FL Studio and other music software)"; Types: full plugin custom; Flags: fixed
Name: "standalone"; Description: "LAPiZ standalone app (plays without FL Studio)"; Types: full

[InstallDelete]
; Start from clean folders, so every file below is created fresh and picks up the permissions in [Dirs].
Type: filesandordirs; Name: "{commoncf64}\VST3\LAPiZ.vst3"
Type: files; Name: "{app}\LAPiZ.exe"
Type: files; Name: "{app}\LICENSE.txt"
Type: files; Name: "{app}\THIRD_PARTY_NOTICES.txt"
Type: files; Name: "{app}\lapiz-version.txt"
Type: files; Name: "{app}\*.old-*"

[Dirs]
; LAPiZ updates itself from GitHub (see Source/Updater.cpp). Letting normal users change these two
; folders means those updates never need an administrator prompt. Nothing outside them is affected.
Name: "{commoncf64}\VST3\LAPiZ.vst3"; Permissions: users-modify
Name: "{app}"; Permissions: users-modify

[Files]
; The VST3 plugin goes in the standard VST3 folder that FL Studio scans: C:\Program Files\Common Files\VST3
Source: "{#Artefacts}\VST3\LAPiZ.vst3\*"; DestDir: "{commoncf64}\VST3\LAPiZ.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\LAPiZ.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion
; Written by the release workflow; tells the updater which version is installed.
Source: "{#Artefacts}\lapiz-version.txt"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion
Source: "..\THIRD_PARTY_NOTICES.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\LAPiZ"; Filename: "{app}\LAPiZ.exe"; Components: standalone

[UninstallDelete]
; Files added or set aside by automatic updates, which the uninstaller doesn't otherwise know about.
Type: filesandordirs; Name: "{commoncf64}\VST3\LAPiZ.vst3"
Type: files; Name: "{app}\*.old-*"
Type: files; Name: "{app}\lapiz-version.txt"

[Messages]
FinishedLabel=LAPiZ is installed, and will keep itself up to date.%n%nIn FL Studio, open Options > Manage plugins, click Find installed plugins, then star LAPiZ to add it to your plugin list.
